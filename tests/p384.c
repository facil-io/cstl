/* *****************************************************************************
ECDSA P-384 (secp384r1) Tests
***************************************************************************** */
#include "test-helpers.h"

#define FIO_P384
#define FIO_SHA2
#define FIO_RAND
#define FIO_LOG
#include FIO_INCLUDE_FILE

/* *****************************************************************************
Helper to print field elements for debugging
***************************************************************************** */
FIO_SFUNC void print_fe384(const char *name, const uint64_t fe[6]) {
  if (FIO_LOG_LEVEL < FIO_LOG_LEVEL_DEBUG)
    return;
  fprintf(stderr, "%s: ", name);
  for (int i = 5; i >= 0; --i) {
    fprintf(stderr, "%016llx", (unsigned long long)fe[i]);
  }
  fprintf(stderr, "\n");
}

FIO_SFUNC void print_scalar384(const char *name, const uint64_t s[6]) {
  if (FIO_LOG_LEVEL < FIO_LOG_LEVEL_DEBUG)
    return;
  fprintf(stderr, "%s: ", name);
  for (int i = 5; i >= 0; --i) {
    fprintf(stderr, "%016llx", (unsigned long long)s[i]);
  }
  fprintf(stderr, "\n");
}

/* *****************************************************************************
Test P-384 curve constants
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, p384_constants)(void) {
  /* Verify prime p = 2^384 - 2^128 - 2^96 + 2^32 - 1 */
  /* p = 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFE
   *     FFFFFFFF0000000000000000FFFFFFFF */
  FIO_ASSERT(FIO___P384_P[0] == 0x00000000FFFFFFFFULL, "P[0] incorrect");
  FIO_ASSERT(FIO___P384_P[1] == 0xFFFFFFFF00000000ULL, "P[1] incorrect");
  FIO_ASSERT(FIO___P384_P[2] == 0xFFFFFFFFFFFFFFFEULL, "P[2] incorrect");
  FIO_ASSERT(FIO___P384_P[3] == 0xFFFFFFFFFFFFFFFFULL, "P[3] incorrect");
  FIO_ASSERT(FIO___P384_P[4] == 0xFFFFFFFFFFFFFFFFULL, "P[4] incorrect");
  FIO_ASSERT(FIO___P384_P[5] == 0xFFFFFFFFFFFFFFFFULL, "P[5] incorrect");

  /* Verify order n */
  /* n = 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFC7634D81F4372DDF
   *     581A0DB248B0A77AECEC196ACCC52973 */
  FIO_ASSERT(FIO___P384_N[0] == 0xECEC196ACCC52973ULL, "N[0] incorrect");
  FIO_ASSERT(FIO___P384_N[1] == 0x581A0DB248B0A77AULL, "N[1] incorrect");
  FIO_ASSERT(FIO___P384_N[2] == 0xC7634D81F4372DDFULL, "N[2] incorrect");
  FIO_ASSERT(FIO___P384_N[3] == 0xFFFFFFFFFFFFFFFFULL, "N[3] incorrect");
  FIO_ASSERT(FIO___P384_N[4] == 0xFFFFFFFFFFFFFFFFULL, "N[4] incorrect");
  FIO_ASSERT(FIO___P384_N[5] == 0xFFFFFFFFFFFFFFFFULL, "N[5] incorrect");
}

/* *****************************************************************************
Test field arithmetic
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, p384_field_ops)(void) {
  fio___p384_fe_s a, b, c;

  /* Test addition: 1 + 2 = 3 */
  fio___p384_fe_one(a);
  b[0] = 2;
  b[1] = b[2] = b[3] = b[4] = b[5] = 0;
  fio___p384_fe_add(c, a, b);
  FIO_ASSERT(c[0] == 3 && c[1] == 0 && c[2] == 0 && c[3] == 0 && c[4] == 0 &&
                 c[5] == 0,
             "Field addition 1+2 failed");

  /* Test subtraction: 5 - 3 = 2 */
  a[0] = 5;
  a[1] = a[2] = a[3] = a[4] = a[5] = 0;
  b[0] = 3;
  b[1] = b[2] = b[3] = b[4] = b[5] = 0;
  fio___p384_fe_sub(c, a, b);
  FIO_ASSERT(c[0] == 2 && c[1] == 0 && c[2] == 0 && c[3] == 0 && c[4] == 0 &&
                 c[5] == 0,
             "Field subtraction 5-3 failed");

  /* Test multiplication: 2 * 3 = 6 */
  a[0] = 2;
  a[1] = a[2] = a[3] = a[4] = a[5] = 0;
  b[0] = 3;
  b[1] = b[2] = b[3] = b[4] = b[5] = 0;
  fio___p384_fe_mul(c, a, b);
  FIO_ASSERT(c[0] == 6 && c[1] == 0 && c[2] == 0 && c[3] == 0 && c[4] == 0 &&
                 c[5] == 0,
             "Field multiplication 2*3 failed");

  /* Test squaring: 4^2 = 16 */
  a[0] = 4;
  a[1] = a[2] = a[3] = a[4] = a[5] = 0;
  fio___p384_fe_sqr(c, a);
  FIO_ASSERT(c[0] == 16 && c[1] == 0 && c[2] == 0 && c[3] == 0 && c[4] == 0 &&
                 c[5] == 0,
             "Field squaring 4^2 failed");

  /* Test inversion: a * a^(-1) = 1 */
  a[0] = 7;
  a[1] = a[2] = a[3] = a[4] = a[5] = 0;
  fio___p384_fe_inv(b, a);
  fio___p384_fe_mul(c, a, b);
  print_fe384("7^(-1)", b);
  print_fe384("7 * 7^(-1)", c);
  FIO_ASSERT(c[0] == 1 && c[1] == 0 && c[2] == 0 && c[3] == 0 && c[4] == 0 &&
                 c[5] == 0,
             "Field inversion 7 * 7^(-1) != 1");

  /* Test inversion with a larger number */
  fio___p384_fe_copy(a, FIO___P384_GX);
  fio___p384_fe_inv(b, a);
  fio___p384_fe_mul(c, a, b);
  print_fe384("Gx^(-1)", b);
  print_fe384("Gx * Gx^(-1)", c);
  FIO_ASSERT(c[0] == 1 && c[1] == 0 && c[2] == 0 && c[3] == 0 && c[4] == 0 &&
                 c[5] == 0,
             "Field inversion Gx * Gx^(-1) != 1");
}

/* *****************************************************************************
Test that base point G is on the curve
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, p384_base_point)(void) {
  fio___p384_fe_s y2, x3, t, x2;

  /* Compute y^2 */
  fio___p384_fe_sqr(y2, FIO___P384_GY);
  print_fe384("y^2", y2);

  /* Compute x^2 */
  fio___p384_fe_sqr(x2, FIO___P384_GX);
  print_fe384("x^2", x2);

  /* Compute x^3 */
  fio___p384_fe_mul(x3, x2, FIO___P384_GX);
  print_fe384("x^3", x3);

  /* Compute x^3 - 3x */
  fio___p384_fe_sub(t, x3, FIO___P384_GX);
  fio___p384_fe_sub(t, t, FIO___P384_GX);
  fio___p384_fe_sub(t, t, FIO___P384_GX);
  print_fe384("x^3-3x", t);

  /* Compute x^3 - 3x + b */
  fio___p384_fe_add(t, t, FIO___P384_B);
  print_fe384("x^3-3x+b", t);

  /* Verify y^2 == x^3 - 3x + b */
  FIO_ASSERT(fio___p384_fe_eq(y2, t) == 0,
             "Base point G is not on the P-384 curve!");
}

/* *****************************************************************************
Test scalar multiplication with base point
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, p384_scalar_mul)(void) {
  /* Test: 1 * G = G */
  fio___p384_scalar_s one = {1, 0, 0, 0, 0, 0};
  fio___p384_point_affine_s g;
  fio___p384_fe_copy(g.x, FIO___P384_GX);
  fio___p384_fe_copy(g.y, FIO___P384_GY);

  fio___p384_point_jacobian_s result;
  fio___p384_point_mul(&result, one, &g);

  fio___p384_point_affine_s result_affine;
  fio___p384_point_to_affine(&result_affine, &result);

  print_fe384("1*G.x", result_affine.x);
  print_fe384("1*G.y", result_affine.y);
  print_fe384("G.x  ", FIO___P384_GX);
  print_fe384("G.y  ", FIO___P384_GY);

  FIO_ASSERT(fio___p384_fe_eq(result_affine.x, FIO___P384_GX) == 0,
             "1*G x-coordinate mismatch");
  FIO_ASSERT(fio___p384_fe_eq(result_affine.y, FIO___P384_GY) == 0,
             "1*G y-coordinate mismatch");

  /* Test: 2 * G using point_double directly */
  fio___p384_point_jacobian_s g_jac, doubled;
  fio___p384_point_to_jacobian(&g_jac, &g);
  fio___p384_point_double(&doubled, &g_jac);
  fio___p384_point_to_affine(&result_affine, &doubled);

  print_fe384("2G.x (double)", result_affine.x);
  print_fe384("2G.y (double)", result_affine.y);

  /* 2G should be on the curve */
  fio___p384_fe_s y2, x3, t;
  fio___p384_fe_sqr(y2, result_affine.y);
  fio___p384_fe_sqr(t, result_affine.x);
  fio___p384_fe_mul(x3, t, result_affine.x);
  fio___p384_fe_sub(t, x3, result_affine.x);
  fio___p384_fe_sub(t, t, result_affine.x);
  fio___p384_fe_sub(t, t, result_affine.x);
  fio___p384_fe_add(t, t, FIO___P384_B);

  print_fe384("2G: y^2", y2);
  print_fe384("2G: x^3-3x+b", t);

  FIO_ASSERT(fio___p384_fe_eq(y2, t) == 0,
             "2*G (doubled) is not on the curve!");

  /* Test: 2 * G via scalar multiplication */
  fio___p384_scalar_s two = {2, 0, 0, 0, 0, 0};
  fio___p384_point_mul(&result, two, &g);
  fio___p384_point_to_affine(&result_affine, &result);

  print_fe384("2G.x (scalar)", result_affine.x);
  print_fe384("2G.y (scalar)", result_affine.y);

  /* 2G should be on the curve */
  fio___p384_fe_sqr(y2, result_affine.y);
  fio___p384_fe_sqr(t, result_affine.x);
  fio___p384_fe_mul(x3, t, result_affine.x);
  fio___p384_fe_sub(t, x3, result_affine.x);
  fio___p384_fe_sub(t, t, result_affine.x);
  fio___p384_fe_sub(t, t, result_affine.x);
  fio___p384_fe_add(t, t, FIO___P384_B);
  FIO_ASSERT(fio___p384_fe_eq(y2, t) == 0, "2*G is not on the curve!");
}

/* *****************************************************************************
NIST ECDSA P-384 Test Vectors
From: https://csrc.nist.gov/projects/cryptographic-algorithm-validation-program
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, p384_ecdsa_nist)(void) {
  /* NIST CAVP ECDSA P-384 SHA-384 test vector (PKV.rsp) */
  /* clang-format off */

  /* Test vector from NIST CAVP SigVer.rsp for P-384 SHA-384 */
  /* Msg = "sample" (SHA-384 hash computed below) */

  /* SHA-384("sample") */
  static const uint8_t msg_hash[48] = {
    0x9a, 0x90, 0x83, 0x50, 0x5b, 0xc9, 0x22, 0x76,
    0xae, 0xc4, 0xbe, 0x31, 0x26, 0x96, 0xef, 0x7b,
    0xf3, 0xbf, 0x60, 0x3f, 0x4b, 0xbd, 0x38, 0x11,
    0x96, 0xa0, 0x29, 0xf3, 0x40, 0x58, 0x53, 0x12,
    0x31, 0x3b, 0xca, 0x4a, 0x9b, 0x5b, 0x89, 0x0e,
    0xfe, 0xe4, 0x2c, 0x77, 0xb1, 0xee, 0x25, 0xfe
  };

  /* Public key from RFC 6979 A.2.6 P-384 test vector */
  /* Qx = EC3A4E415B4E19A4568618029F427FA5DA9A8BC4AE92E02E06AAE5286B300C64
   *      DEF8F0EA9055866064A254515480BC13 */
  /* Qy = 8015D9B72D7D57244EA8EF9AC0C621896708A59367F9DFB9F54CA84B3F1C9DB1
   *      288B231C3AE0D4FE7344FD2533264720 */
  static const uint8_t pubkey[97] = {
    0x04,
    /* X coordinate (48 bytes) */
    0xec, 0x3a, 0x4e, 0x41, 0x5b, 0x4e, 0x19, 0xa4,
    0x56, 0x86, 0x18, 0x02, 0x9f, 0x42, 0x7f, 0xa5,
    0xda, 0x9a, 0x8b, 0xc4, 0xae, 0x92, 0xe0, 0x2e,
    0x06, 0xaa, 0xe5, 0x28, 0x6b, 0x30, 0x0c, 0x64,
    0xde, 0xf8, 0xf0, 0xea, 0x90, 0x55, 0x86, 0x60,
    0x64, 0xa2, 0x54, 0x51, 0x54, 0x80, 0xbc, 0x13,
    /* Y coordinate (48 bytes) */
    0x80, 0x15, 0xd9, 0xb7, 0x2d, 0x7d, 0x57, 0x24,
    0x4e, 0xa8, 0xef, 0x9a, 0xc0, 0xc6, 0x21, 0x89,
    0x67, 0x08, 0xa5, 0x93, 0x67, 0xf9, 0xdf, 0xb9,
    0xf5, 0x4c, 0xa8, 0x4b, 0x3f, 0x1c, 0x9d, 0xb1,
    0x28, 0x8b, 0x23, 0x1c, 0x3a, 0xe0, 0xd4, 0xfe,
    0x73, 0x44, 0xfd, 0x25, 0x33, 0x26, 0x47, 0x20
  };

  /* Signature from RFC 6979 A.2.6 P-384 test vector */
  /* R = 94EDBB92A5ECB8AAD4736E56C691916B3F88140666CE9FA73D64C4EA95AD133C
   *     81A648152E44ACF96E36DD1E80FABE46 */
  /* S = 99EF4AEB15F178CEA1FE40DB2603138F130E740A19624526203B6351D0A3A94F
   *     A329C145786E679E7B82C71A38628AC8 */
  static const uint8_t r[48] = {
    0x94, 0xed, 0xbb, 0x92, 0xa5, 0xec, 0xb8, 0xaa,
    0xd4, 0x73, 0x6e, 0x56, 0xc6, 0x91, 0x91, 0x6b,
    0x3f, 0x88, 0x14, 0x06, 0x66, 0xce, 0x9f, 0xa7,
    0x3d, 0x64, 0xc4, 0xea, 0x95, 0xad, 0x13, 0x3c,
    0x81, 0xa6, 0x48, 0x15, 0x2e, 0x44, 0xac, 0xf9,
    0x6e, 0x36, 0xdd, 0x1e, 0x80, 0xfa, 0xbe, 0x46
  };
  static const uint8_t s[48] = {
    0x99, 0xef, 0x4a, 0xeb, 0x15, 0xf1, 0x78, 0xce,
    0xa1, 0xfe, 0x40, 0xdb, 0x26, 0x03, 0x13, 0x8f,
    0x13, 0x0e, 0x74, 0x0a, 0x19, 0x62, 0x45, 0x26,
    0x20, 0x3b, 0x63, 0x51, 0xd0, 0xa3, 0xa9, 0x4f,
    0xa3, 0x29, 0xc1, 0x45, 0x78, 0x6e, 0x67, 0x9e,
    0x7b, 0x82, 0xc7, 0x1a, 0x38, 0x62, 0x8a, 0xc8
  };
  /* clang-format on */

  /* Debug: trace the verification */
  fio___p384_scalar_s r_scalar, s_scalar, e;
  fio___p384_point_affine_s q;

  /* Load signature components */
  fio___p384_scalar_from_bytes(r_scalar, r);
  fio___p384_scalar_from_bytes(s_scalar, s);
  fio___p384_scalar_from_bytes(e, msg_hash);

  print_scalar384("r", r_scalar);
  print_scalar384("s", s_scalar);
  print_scalar384("e (hash)", e);

  /* Load public key */
  fio___p384_fe_from_bytes(q.x, pubkey + 1);
  fio___p384_fe_from_bytes(q.y, pubkey + 49);
  print_fe384("Q.x", q.x);
  print_fe384("Q.y", q.y);

  /* Verify Q is on curve */
  {
    fio___p384_fe_s y2, x3, t;
    fio___p384_fe_sqr(y2, q.y);
    fio___p384_fe_sqr(t, q.x);
    fio___p384_fe_mul(x3, t, q.x);
    fio___p384_fe_sub(t, x3, q.x);
    fio___p384_fe_sub(t, t, q.x);
    fio___p384_fe_sub(t, t, q.x);
    fio___p384_fe_add(t, t, FIO___P384_B);
    fprintf(stderr,
            "Q on curve: %s\n",
            fio___p384_fe_eq(y2, t) == 0 ? "YES" : "NO");
    FIO_ASSERT(fio___p384_fe_eq(y2, t) == 0, "Public key Q is not on curve!");
  }

  /* Compute w = s^(-1) mod n */
  fio___p384_scalar_s w;
  fio___p384_scalar_inv(w, s_scalar);
  print_scalar384("w = s^(-1)", w);

  /* Verify: s * w should equal 1 */
  fio___p384_scalar_s check;
  fio___p384_scalar_mul(check, s_scalar, w);
  print_scalar384("s * w (should be 1)", check);
  FIO_ASSERT(check[0] == 1 && check[1] == 0 && check[2] == 0 && check[3] == 0 &&
                 check[4] == 0 && check[5] == 0,
             "s * s^(-1) should be 1");

  /* Compute u1 = e * w mod n */
  fio___p384_scalar_s u1;
  fio___p384_scalar_mul(u1, e, w);
  print_scalar384("u1 = e*w", u1);

  /* Compute u2 = r * w mod n */
  fio___p384_scalar_s u2;
  fio___p384_scalar_mul(u2, r_scalar, w);
  print_scalar384("u2 = r*w", u2);

  /* Compute R = u1*G + u2*Q */
  fio___p384_point_jacobian_s R_jac;
  fio___p384_point_mul2(&R_jac, u1, u2, &q);

  /* Convert R to affine */
  fio___p384_point_affine_s R_aff;
  fio___p384_point_to_affine(&R_aff, &R_jac);
  print_fe384("R.x", R_aff.x);
  print_fe384("R.y", R_aff.y);

  /* Get R.x as bytes and load as scalar */
  uint8_t rx_bytes[48];
  fio___p384_fe_to_bytes(rx_bytes, R_aff.x);
  fio___p384_scalar_s rx;
  fio___p384_scalar_from_bytes(rx, rx_bytes);
  print_scalar384("R.x as scalar", rx);

  /* Reduce mod n if needed */
  while (fio___p384_scalar_gte_n(rx)) {
    uint64_t borrow = 0;
    rx[0] = fio_math_subc64(rx[0], FIO___P384_N[0], 0, &borrow);
    rx[1] = fio_math_subc64(rx[1], FIO___P384_N[1], borrow, &borrow);
    rx[2] = fio_math_subc64(rx[2], FIO___P384_N[2], borrow, &borrow);
    rx[3] = fio_math_subc64(rx[3], FIO___P384_N[3], borrow, &borrow);
    rx[4] = fio_math_subc64(rx[4], FIO___P384_N[4], borrow, &borrow);
    rx[5] = fio_math_subc64(rx[5], FIO___P384_N[5], borrow, &borrow);
  }
  print_scalar384("R.x mod n", rx);
  print_scalar384("r (expected)", r_scalar);

  /* Test with raw r,s values */
  int result =
      fio_ecdsa_p384_verify_raw(r, s, msg_hash, pubkey + 1, pubkey + 49);
  FIO_ASSERT(result == 0, "NIST ECDSA P-384 test vector failed");
}

/* *****************************************************************************
Test DER signature parsing
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, p384_der_parsing)(void) {
  /* Create a DER-encoded signature from RFC 6979 test vector */
  /* clang-format off */

  /* SHA-384("sample") */
  static const uint8_t msg_hash[48] = {
    0x9a, 0x90, 0x83, 0x50, 0x5b, 0xc9, 0x22, 0x76,
    0xae, 0xc4, 0xbe, 0x31, 0x26, 0x96, 0xef, 0x7b,
    0xf3, 0xbf, 0x60, 0x3f, 0x4b, 0xbd, 0x38, 0x11,
    0x96, 0xa0, 0x29, 0xf3, 0x40, 0x58, 0x53, 0x12,
    0x31, 0x3b, 0xca, 0x4a, 0x9b, 0x5b, 0x89, 0x0e,
    0xfe, 0xe4, 0x2c, 0x77, 0xb1, 0xee, 0x25, 0xfe
  };

  static const uint8_t pubkey[97] = {
    0x04,
    /* X coordinate (48 bytes) */
    0xec, 0x3a, 0x4e, 0x41, 0x5b, 0x4e, 0x19, 0xa4,
    0x56, 0x86, 0x18, 0x02, 0x9f, 0x42, 0x7f, 0xa5,
    0xda, 0x9a, 0x8b, 0xc4, 0xae, 0x92, 0xe0, 0x2e,
    0x06, 0xaa, 0xe5, 0x28, 0x6b, 0x30, 0x0c, 0x64,
    0xde, 0xf8, 0xf0, 0xea, 0x90, 0x55, 0x86, 0x60,
    0x64, 0xa2, 0x54, 0x51, 0x54, 0x80, 0xbc, 0x13,
    /* Y coordinate (48 bytes) */
    0x80, 0x15, 0xd9, 0xb7, 0x2d, 0x7d, 0x57, 0x24,
    0x4e, 0xa8, 0xef, 0x9a, 0xc0, 0xc6, 0x21, 0x89,
    0x67, 0x08, 0xa5, 0x93, 0x67, 0xf9, 0xdf, 0xb9,
    0xf5, 0x4c, 0xa8, 0x4b, 0x3f, 0x1c, 0x9d, 0xb1,
    0x28, 0x8b, 0x23, 0x1c, 0x3a, 0xe0, 0xd4, 0xfe,
    0x73, 0x44, 0xfd, 0x25, 0x33, 0x26, 0x47, 0x20
  };

  /* DER-encoded signature */
  static const uint8_t der_sig[] = {
    0x30, 0x65, /* SEQUENCE, length 101 */
    0x02, 0x31, /* INTEGER, length 49 */
    0x00, /* Leading zero (R >= 0x80) */
    0x94, 0xed, 0xbb, 0x92, 0xa5, 0xec, 0xb8, 0xaa,
    0xd4, 0x73, 0x6e, 0x56, 0xc6, 0x91, 0x91, 0x6b,
    0x3f, 0x88, 0x14, 0x06, 0x66, 0xce, 0x9f, 0xa7,
    0x3d, 0x64, 0xc4, 0xea, 0x95, 0xad, 0x13, 0x3c,
    0x81, 0xa6, 0x48, 0x15, 0x2e, 0x44, 0xac, 0xf9,
    0x6e, 0x36, 0xdd, 0x1e, 0x80, 0xfa, 0xbe, 0x46,
    0x02, 0x30, /* INTEGER, length 48 */
    0x99, 0xef, 0x4a, 0xeb, 0x15, 0xf1, 0x78, 0xce,
    0xa1, 0xfe, 0x40, 0xdb, 0x26, 0x03, 0x13, 0x8f,
    0x13, 0x0e, 0x74, 0x0a, 0x19, 0x62, 0x45, 0x26,
    0x20, 0x3b, 0x63, 0x51, 0xd0, 0xa3, 0xa9, 0x4f,
    0xa3, 0x29, 0xc1, 0x45, 0x78, 0x6e, 0x67, 0x9e,
    0x7b, 0x82, 0xc7, 0x1a, 0x38, 0x62, 0x8a, 0xc8
  };
  /* clang-format on */

  int result =
      fio_ecdsa_p384_verify(der_sig, sizeof(der_sig), msg_hash, pubkey, 97);
  FIO_ASSERT(result == 0, "DER signature verification failed");
}

/* *****************************************************************************
Test invalid signatures
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, p384_invalid_sigs)(void) {
  /* clang-format off */
  static const uint8_t msg_hash[48] = {
    0x9a, 0x90, 0x83, 0x50, 0x5b, 0xc9, 0x22, 0x76,
    0xae, 0xc4, 0xbe, 0x31, 0x26, 0x96, 0xef, 0x7b,
    0xf3, 0xbf, 0x60, 0x3f, 0x4b, 0xbd, 0x38, 0x11,
    0x96, 0xa0, 0x29, 0xf3, 0x40, 0x58, 0x53, 0x12,
    0x31, 0x3b, 0xca, 0x4a, 0x9b, 0x5b, 0x89, 0x0e,
    0xfe, 0xe4, 0x2c, 0x77, 0xb1, 0xee, 0x25, 0xfe
  };

  static const uint8_t pubkey[97] = {
    0x04,
    0xec, 0x3a, 0x4e, 0x41, 0x5b, 0x4e, 0x19, 0xa4,
    0x56, 0x86, 0x18, 0x02, 0x9f, 0x42, 0x7f, 0xa5,
    0xda, 0x9a, 0x8b, 0xc4, 0xae, 0x92, 0xe0, 0x2e,
    0x06, 0xaa, 0xe5, 0x28, 0x6b, 0x30, 0x0c, 0x64,
    0xde, 0xf8, 0xf0, 0xea, 0x90, 0x55, 0x86, 0x60,
    0x64, 0xa2, 0x54, 0x51, 0x54, 0x80, 0xbc, 0x13,
    0x80, 0x15, 0xd9, 0xb7, 0x2d, 0x7d, 0x57, 0x24,
    0x4e, 0xa8, 0xef, 0x9a, 0xc0, 0xc6, 0x21, 0x89,
    0x67, 0x08, 0xa5, 0x93, 0x67, 0xf9, 0xdf, 0xb9,
    0xf5, 0x4c, 0xa8, 0x4b, 0x3f, 0x1c, 0x9d, 0xb1,
    0x28, 0x8b, 0x23, 0x1c, 0x3a, 0xe0, 0xd4, 0xfe,
    0x73, 0x44, 0xfd, 0x25, 0x33, 0x26, 0x47, 0x20
  };

  static const uint8_t r[48] = {
    0x94, 0xed, 0xbb, 0x92, 0xa5, 0xec, 0xb8, 0xaa,
    0xd4, 0x73, 0x6e, 0x56, 0xc6, 0x91, 0x91, 0x6b,
    0x3f, 0x88, 0x14, 0x06, 0x66, 0xce, 0x9f, 0xa7,
    0x3d, 0x64, 0xc4, 0xea, 0x95, 0xad, 0x13, 0x3c,
    0x81, 0xa6, 0x48, 0x15, 0x2e, 0x44, 0xac, 0xf9,
    0x6e, 0x36, 0xdd, 0x1e, 0x80, 0xfa, 0xbe, 0x46
  };

  static const uint8_t s[48] = {
    0x99, 0xef, 0x4a, 0xeb, 0x15, 0xf1, 0x78, 0xce,
    0xa1, 0xfe, 0x40, 0xdb, 0x26, 0x03, 0x13, 0x8f,
    0x13, 0x0e, 0x74, 0x0a, 0x19, 0x62, 0x45, 0x26,
    0x20, 0x3b, 0x63, 0x51, 0xd0, 0xa3, 0xa9, 0x4f,
    0xa3, 0x29, 0xc1, 0x45, 0x78, 0x6e, 0x67, 0x9e,
    0x7b, 0x82, 0xc7, 0x1a, 0x38, 0x62, 0x8a, 0xc8
  };
  /* clang-format on */

  /* First verify the valid signature works */
  int result =
      fio_ecdsa_p384_verify_raw(r, s, msg_hash, pubkey + 1, pubkey + 49);
  FIO_ASSERT(result == 0, "Valid signature should verify");

  /* Test 1: Modified r should fail */
  uint8_t bad_r[48];
  FIO_MEMCPY(bad_r, r, 48);
  bad_r[0] ^= 0x01;
  result =
      fio_ecdsa_p384_verify_raw(bad_r, s, msg_hash, pubkey + 1, pubkey + 49);
  FIO_ASSERT(result != 0, "Should reject modified r");

  /* Test 2: Modified s should fail */
  uint8_t bad_s[48];
  FIO_MEMCPY(bad_s, s, 48);
  bad_s[0] ^= 0x01;
  result =
      fio_ecdsa_p384_verify_raw(r, bad_s, msg_hash, pubkey + 1, pubkey + 49);
  FIO_ASSERT(result != 0, "Should reject modified s");

  /* Test 3: Modified message hash should fail */
  uint8_t bad_hash[48];
  FIO_MEMCPY(bad_hash, msg_hash, 48);
  bad_hash[0] ^= 0x01;
  result = fio_ecdsa_p384_verify_raw(r, s, bad_hash, pubkey + 1, pubkey + 49);
  FIO_ASSERT(result != 0, "Should reject modified message hash");

  /* Test 4: r = 0 should fail */
  uint8_t zero[48] = {0};
  result =
      fio_ecdsa_p384_verify_raw(zero, s, msg_hash, pubkey + 1, pubkey + 49);
  FIO_ASSERT(result != 0, "Should reject r = 0");

  /* Test 5: s = 0 should fail */
  result =
      fio_ecdsa_p384_verify_raw(r, zero, msg_hash, pubkey + 1, pubkey + 49);
  FIO_ASSERT(result != 0, "Should reject s = 0");
}

/* *****************************************************************************
Performance test
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, p384_performance)(void) {
#ifdef DEBUG
  return;
#endif
  /* clang-format off */
  static const uint8_t msg_hash[48] = {
    0x9a, 0x90, 0x83, 0x50, 0x5b, 0xc9, 0x22, 0x76,
    0xae, 0xc4, 0xbe, 0x31, 0x26, 0x96, 0xef, 0x7b,
    0xf3, 0xbf, 0x60, 0x3f, 0x4b, 0xbd, 0x38, 0x11,
    0x96, 0xa0, 0x29, 0xf3, 0x40, 0x58, 0x53, 0x12,
    0x31, 0x3b, 0xca, 0x4a, 0x9b, 0x5b, 0x89, 0x0e,
    0xfe, 0xe4, 0x2c, 0x77, 0xb1, 0xee, 0x25, 0xfe
  };

  static const uint8_t pubkey[97] = {
    0x04,
    0xec, 0x3a, 0x4e, 0x41, 0x5b, 0x4e, 0x19, 0xa4,
    0x56, 0x86, 0x18, 0x02, 0x9f, 0x42, 0x7f, 0xa5,
    0xda, 0x9a, 0x8b, 0xc4, 0xae, 0x92, 0xe0, 0x2e,
    0x06, 0xaa, 0xe5, 0x28, 0x6b, 0x30, 0x0c, 0x64,
    0xde, 0xf8, 0xf0, 0xea, 0x90, 0x55, 0x86, 0x60,
    0x64, 0xa2, 0x54, 0x51, 0x54, 0x80, 0xbc, 0x13,
    0x80, 0x15, 0xd9, 0xb7, 0x2d, 0x7d, 0x57, 0x24,
    0x4e, 0xa8, 0xef, 0x9a, 0xc0, 0xc6, 0x21, 0x89,
    0x67, 0x08, 0xa5, 0x93, 0x67, 0xf9, 0xdf, 0xb9,
    0xf5, 0x4c, 0xa8, 0x4b, 0x3f, 0x1c, 0x9d, 0xb1,
    0x28, 0x8b, 0x23, 0x1c, 0x3a, 0xe0, 0xd4, 0xfe,
    0x73, 0x44, 0xfd, 0x25, 0x33, 0x26, 0x47, 0x20
  };

  static const uint8_t r[48] = {
    0x94, 0xed, 0xbb, 0x92, 0xa5, 0xec, 0xb8, 0xaa,
    0xd4, 0x73, 0x6e, 0x56, 0xc6, 0x91, 0x91, 0x6b,
    0x3f, 0x88, 0x14, 0x06, 0x66, 0xce, 0x9f, 0xa7,
    0x3d, 0x64, 0xc4, 0xea, 0x95, 0xad, 0x13, 0x3c,
    0x81, 0xa6, 0x48, 0x15, 0x2e, 0x44, 0xac, 0xf9,
    0x6e, 0x36, 0xdd, 0x1e, 0x80, 0xfa, 0xbe, 0x46
  };

  static const uint8_t s[48] = {
    0x99, 0xef, 0x4a, 0xeb, 0x15, 0xf1, 0x78, 0xce,
    0xa1, 0xfe, 0x40, 0xdb, 0x26, 0x03, 0x13, 0x8f,
    0x13, 0x0e, 0x74, 0x0a, 0x19, 0x62, 0x45, 0x26,
    0x20, 0x3b, 0x63, 0x51, 0xd0, 0xa3, 0xa9, 0x4f,
    0xa3, 0x29, 0xc1, 0x45, 0x78, 0x6e, 0x67, 0x9e,
    0x7b, 0x82, 0xc7, 0x1a, 0x38, 0x62, 0x8a, 0xc8
  };
  /* clang-format on */

  const size_t iterations = 50;
  struct timespec start, end;

  clock_gettime(CLOCK_MONOTONIC, &start);
  for (size_t i = 0; i < iterations; ++i) {
    volatile int result =
        fio_ecdsa_p384_verify_raw(r, s, msg_hash, pubkey + 1, pubkey + 49);
    (void)result;
    FIO_COMPILER_GUARD;
  }
  clock_gettime(CLOCK_MONOTONIC, &end);

  double elapsed_us = (double)(end.tv_sec - start.tv_sec) * 1000000.0 +
                      (double)(end.tv_nsec - start.tv_nsec) / 1000.0;
  double ops_per_sec = (iterations * 1000000.0) / elapsed_us;

  fprintf(stderr,
          "\t  ECDSA P-384 verify: %.2f ops/sec (%.2f us/op)\n",
          ops_per_sec,
          elapsed_us / iterations);
}

/* *****************************************************************************
Main Test Function
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, p384)(void) {
  FIO_NAME_TEST(stl, p384_constants)();
  FIO_NAME_TEST(stl, p384_field_ops)();
  FIO_NAME_TEST(stl, p384_base_point)();
  FIO_NAME_TEST(stl, p384_scalar_mul)();
  FIO_NAME_TEST(stl, p384_ecdsa_nist)();
  FIO_NAME_TEST(stl, p384_der_parsing)();
  FIO_NAME_TEST(stl, p384_invalid_sigs)();
  FIO_NAME_TEST(stl, p384_performance)();
}

/* *****************************************************************************
Test Runner
***************************************************************************** */

int main(int argc, char const *argv[]) {
  (void)argc;
  (void)argv;
  FIO_NAME_TEST(stl, p384)();
  return 0;
}
