/* *****************************************************************************
ECDSA P-256 (secp256r1) Tests
***************************************************************************** */
#include "test-helpers.h"

#define FIO_P256
#define FIO_SHA2
#define FIO_RAND
#define FIO_LOG
#include FIO_INCLUDE_FILE

/* *****************************************************************************
Helper to print field elements for debugging
***************************************************************************** */
FIO_SFUNC void print_fe(const char *name, const uint64_t fe[4]) {
  fprintf(stderr, "%s: ", name);
  for (int i = 3; i >= 0; --i) {
    fprintf(stderr, "%016llx", (unsigned long long)fe[i]);
  }
  fprintf(stderr, "\n");
}

/* *****************************************************************************
Test P-256 curve constants
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, p256_constants)(void) {
  FIO_LOG_DDEBUG("Testing P-256 curve constants");

  /* Verify prime p = 2^256 - 2^224 + 2^192 + 2^96 - 1 */
  /* p = 0xFFFFFFFF00000001000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFF */
  FIO_ASSERT(FIO___P256_P[0] == 0xFFFFFFFFFFFFFFFFULL, "P[0] incorrect");
  FIO_ASSERT(FIO___P256_P[1] == 0x00000000FFFFFFFFULL, "P[1] incorrect");
  FIO_ASSERT(FIO___P256_P[2] == 0x0000000000000000ULL, "P[2] incorrect");
  FIO_ASSERT(FIO___P256_P[3] == 0xFFFFFFFF00000001ULL, "P[3] incorrect");

  /* Verify order n */
  /* n = 0xFFFFFFFF00000000FFFFFFFFFFFFFFFFBCE6FAADA7179E84F3B9CAC2FC632551 */
  FIO_ASSERT(FIO___P256_N[0] == 0xF3B9CAC2FC632551ULL, "N[0] incorrect");
  FIO_ASSERT(FIO___P256_N[1] == 0xBCE6FAADA7179E84ULL, "N[1] incorrect");
  FIO_ASSERT(FIO___P256_N[2] == 0xFFFFFFFFFFFFFFFFULL, "N[2] incorrect");
  FIO_ASSERT(FIO___P256_N[3] == 0xFFFFFFFF00000000ULL, "N[3] incorrect");

  FIO_LOG_DDEBUG("  Curve constants verified.");
}

/* *****************************************************************************
Test field arithmetic
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, p256_field_ops)(void) {
  FIO_LOG_DDEBUG("Testing P-256 field arithmetic");

  fio___p256_fe_s a, b, c;

  /* Test addition: 1 + 2 = 3 */
  fio___p256_fe_one(a);
  b[0] = 2;
  b[1] = b[2] = b[3] = 0;
  fio___p256_fe_add(c, a, b);
  FIO_ASSERT(c[0] == 3 && c[1] == 0 && c[2] == 0 && c[3] == 0,
             "Field addition 1+2 failed");

  /* Test subtraction: 5 - 3 = 2 */
  a[0] = 5;
  a[1] = a[2] = a[3] = 0;
  b[0] = 3;
  b[1] = b[2] = b[3] = 0;
  fio___p256_fe_sub(c, a, b);
  FIO_ASSERT(c[0] == 2 && c[1] == 0 && c[2] == 0 && c[3] == 0,
             "Field subtraction 5-3 failed");

  /* Test multiplication: 2 * 3 = 6 */
  a[0] = 2;
  a[1] = a[2] = a[3] = 0;
  b[0] = 3;
  b[1] = b[2] = b[3] = 0;
  fio___p256_fe_mul(c, a, b);
  FIO_ASSERT(c[0] == 6 && c[1] == 0 && c[2] == 0 && c[3] == 0,
             "Field multiplication 2*3 failed");

  /* Test squaring: 4^2 = 16 */
  a[0] = 4;
  a[1] = a[2] = a[3] = 0;
  fio___p256_fe_sqr(c, a);
  FIO_ASSERT(c[0] == 16 && c[1] == 0 && c[2] == 0 && c[3] == 0,
             "Field squaring 4^2 failed");

  /* Test inversion: a * a^(-1) = 1 */
  FIO_LOG_DDEBUG("  Testing field inversion...");

  /* First verify squaring works */
  a[0] = 7;
  a[1] = a[2] = a[3] = 0;
  fio___p256_fe_sqr(b, a);
  print_fe("7^2", b);
  FIO_ASSERT(b[0] == 49 && b[1] == 0 && b[2] == 0 && b[3] == 0,
             "7^2 should be 49");

  fio___p256_fe_sqr(c, b);
  print_fe("7^4", c);
  FIO_ASSERT(c[0] == 2401 && c[1] == 0 && c[2] == 0 && c[3] == 0,
             "7^4 should be 2401");

  /* Test in-place squaring (aliasing) */
  a[0] = 7;
  a[1] = a[2] = a[3] = 0;
  fio___p256_fe_sqr(a, a); /* a = a^2 in place */
  print_fe("7^2 (in-place)", a);
  FIO_ASSERT(a[0] == 49 && a[1] == 0 && a[2] == 0 && a[3] == 0,
             "7^2 in-place should be 49");

  fio___p256_fe_sqr(a, a); /* a = a^2 in place again */
  print_fe("7^4 (in-place)", a);
  FIO_ASSERT(a[0] == 2401 && a[1] == 0 && a[2] == 0 && a[3] == 0,
             "7^4 in-place should be 2401");

  /* Test in-place multiplication */
  a[0] = 7;
  a[1] = a[2] = a[3] = 0;
  b[0] = 1;
  b[1] = b[2] = b[3] = 0;
  fio___p256_fe_mul(b, b, a); /* b = b * a = 1 * 7 = 7 */
  print_fe("1*7 (in-place)", b);
  FIO_ASSERT(b[0] == 7 && b[1] == 0 && b[2] == 0 && b[3] == 0,
             "1*7 in-place should be 7");

  fio___p256_fe_mul(b, b, a); /* b = b * a = 7 * 7 = 49 */
  print_fe("7*7 (in-place)", b);
  FIO_ASSERT(b[0] == 49 && b[1] == 0 && b[2] == 0 && b[3] == 0,
             "7*7 in-place should be 49");

  fio___p256_fe_mul(b, b, a); /* b = b * a = 49 * 7 = 343 = 0x157 */
  print_fe("49*7 (in-place)", b);
  FIO_ASSERT(b[0] == 343 && b[1] == 0 && b[2] == 0 && b[3] == 0,
             "49*7 in-place should be 343");

  /* Test: 7 * 7^4 = 7 * 2401 = 16807 = 0x41a7 */
  a[0] = 7;
  a[1] = a[2] = a[3] = 0;
  b[0] = 2401;
  b[1] = b[2] = b[3] = 0;
  fio___p256_fe_mul(c, a, b);
  print_fe("7*2401", c);
  FIO_ASSERT(c[0] == 16807 && c[1] == 0 && c[2] == 0 && c[3] == 0,
             "7*2401 should be 16807");

  /* Test the specific multiplication that fails in inversion:
   * 0x168f08f8e7 * 0x1e39a5057d81 mod p = 0x2a9d770d7203e34913767
   */
  a[0] = 0x168f08f8e7ULL;
  a[1] = a[2] = a[3] = 0;
  b[0] = 0x1e39a5057d81ULL;
  b[1] = b[2] = b[3] = 0;
  fio___p256_fe_mul(c, a, b);
  print_fe("0x168f08f8e7 * 0x1e39a5057d81", c);
  /* Expected: 0x2a9d770d7203e34913767 */
  /* limb 0 = 0x70d7203e34913767, limb 1 = 0x2a9d7 */
  FIO_ASSERT(c[0] == 0x70d7203e34913767ULL && c[1] == 0x2a9d7ULL && c[2] == 0 &&
                 c[3] == 0,
             "Multiplication result incorrect");

  /* Test the specific squaring that fails:
   * base6 = 0x000cbc21fe4561c8d63b78e780e1341e199417c8c0bb7601
   * base6^2 mod p =
   * 0x15b30a3d45561e229b30eca65bf400f16fb4ed6ea70cff34d2b4192c08178833
   */
  a[0] = 0x199417c8c0bb7601ULL;
  a[1] = 0xd63b78e780e1341eULL;
  a[2] = 0x000cbc21fe4561c8ULL;
  a[3] = 0;
  print_fe("base6", a);
  fio___p256_fe_sqr(c, a);
  print_fe("base6^2", c);
  /* Expected:
   * 0x15b30a3d45561e229b30eca65bf400f16fb4ed6ea70cff34d2b4192c08178833 */
  FIO_ASSERT(c[0] == 0xd2b4192c08178833ULL && c[1] == 0x6fb4ed6ea70cff34ULL &&
                 c[2] == 0x9b30eca65bf400f1ULL && c[3] == 0x15b30a3d45561e22ULL,
             "Squaring base6 incorrect");

  /* Now test full inversion */
  a[0] = 7;
  a[1] = a[2] = a[3] = 0;
  fio___p256_fe_inv(b, a);
  fio___p256_fe_mul(c, a, b);
  print_fe("7^(-1)", b);
  print_fe("7 * 7^(-1)", c);
  FIO_ASSERT(c[0] == 1 && c[1] == 0 && c[2] == 0 && c[3] == 0,
             "Field inversion 7 * 7^(-1) != 1");

  /* Test inversion with a larger number */
  fio___p256_fe_copy(a, FIO___P256_GX);
  fio___p256_fe_inv(b, a);
  fio___p256_fe_mul(c, a, b);
  print_fe("Gx^(-1)", b);
  print_fe("Gx * Gx^(-1)", c);
  FIO_ASSERT(c[0] == 1 && c[1] == 0 && c[2] == 0 && c[3] == 0,
             "Field inversion Gx * Gx^(-1) != 1");

  FIO_LOG_DDEBUG("  Field arithmetic tests passed.");
}

/* *****************************************************************************
Test that base point G is on the curve
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, p256_base_point)(void) {
  FIO_LOG_DDEBUG("Testing P-256 base point is on curve");

  fio___p256_fe_s y2, x3, t, x2;

  /* Compute y² */
  fio___p256_fe_sqr(y2, FIO___P256_GY);
  print_fe("y^2", y2);

  /* Compute x² */
  fio___p256_fe_sqr(x2, FIO___P256_GX);
  print_fe("x^2", x2);

  /* Compute x³ */
  fio___p256_fe_mul(x3, x2, FIO___P256_GX);
  print_fe("x^3", x3);

  /* Compute x³ - 3x */
  fio___p256_fe_sub(t, x3, FIO___P256_GX);
  fio___p256_fe_sub(t, t, FIO___P256_GX);
  fio___p256_fe_sub(t, t, FIO___P256_GX);
  print_fe("x^3-3x", t);

  /* Compute x³ - 3x + b */
  fio___p256_fe_add(t, t, FIO___P256_B);
  print_fe("x^3-3x+b", t);

  /* Verify y² == x³ - 3x + b */
  FIO_ASSERT(fio___p256_fe_eq(y2, t) == 0,
             "Base point G is not on the P-256 curve!");

  FIO_LOG_DDEBUG("  Base point verified on curve.");
}

/* *****************************************************************************
Test scalar multiplication with base point
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, p256_scalar_mul)(void) {
  FIO_LOG_DDEBUG("Testing P-256 scalar multiplication");

  /* Test: 1 * G = G */
  fio___p256_scalar_s one = {1, 0, 0, 0};
  fio___p256_point_affine_s g;
  fio___p256_fe_copy(g.x, FIO___P256_GX);
  fio___p256_fe_copy(g.y, FIO___P256_GY);

  fio___p256_point_jacobian_s result;
  fio___p256_point_mul(&result, one, &g);

  fio___p256_point_affine_s result_affine;
  fio___p256_point_to_affine(&result_affine, &result);

  print_fe("1*G.x", result_affine.x);
  print_fe("1*G.y", result_affine.y);
  print_fe("G.x  ", FIO___P256_GX);
  print_fe("G.y  ", FIO___P256_GY);

  FIO_ASSERT(fio___p256_fe_eq(result_affine.x, FIO___P256_GX) == 0,
             "1*G x-coordinate mismatch");
  FIO_ASSERT(fio___p256_fe_eq(result_affine.y, FIO___P256_GY) == 0,
             "1*G y-coordinate mismatch");

  /* Test: 2 * G using point_double directly */
  FIO_LOG_DDEBUG("  Testing point doubling directly...");
  fio___p256_point_jacobian_s g_jac, doubled;
  fio___p256_point_to_jacobian(&g_jac, &g);
  fio___p256_point_double(&doubled, &g_jac);
  fio___p256_point_to_affine(&result_affine, &doubled);

  print_fe("2G.x (double)", result_affine.x);
  print_fe("2G.y (double)", result_affine.y);

  /* 2G should be on the curve */
  fio___p256_fe_s y2, x3, t;
  fio___p256_fe_sqr(y2, result_affine.y);
  fio___p256_fe_sqr(t, result_affine.x);
  fio___p256_fe_mul(x3, t, result_affine.x);
  fio___p256_fe_sub(t, x3, result_affine.x);
  fio___p256_fe_sub(t, t, result_affine.x);
  fio___p256_fe_sub(t, t, result_affine.x);
  fio___p256_fe_add(t, t, FIO___P256_B);

  print_fe("2G: y^2", y2);
  print_fe("2G: x^3-3x+b", t);

  FIO_ASSERT(fio___p256_fe_eq(y2, t) == 0,
             "2*G (doubled) is not on the curve!");

  /* Test: 2 * G via scalar multiplication */
  FIO_LOG_DDEBUG("  Testing 2*G via scalar multiplication...");
  fio___p256_scalar_s two = {2, 0, 0, 0};
  fio___p256_point_mul(&result, two, &g);
  fio___p256_point_to_affine(&result_affine, &result);

  print_fe("2G.x (scalar)", result_affine.x);
  print_fe("2G.y (scalar)", result_affine.y);

  /* 2G should be on the curve */
  fio___p256_fe_sqr(y2, result_affine.y);
  fio___p256_fe_sqr(t, result_affine.x);
  fio___p256_fe_mul(x3, t, result_affine.x);
  fio___p256_fe_sub(t, x3, result_affine.x);
  fio___p256_fe_sub(t, t, result_affine.x);
  fio___p256_fe_sub(t, t, result_affine.x);
  fio___p256_fe_add(t, t, FIO___P256_B);
  FIO_ASSERT(fio___p256_fe_eq(y2, t) == 0, "2*G is not on the curve!");

  FIO_LOG_DDEBUG("  Scalar multiplication tests passed.");
}

/* *****************************************************************************
NIST ECDSA P-256 Test Vectors
From:
https://csrc.nist.gov/groups/STM/cavp/documents/dss/186-3ecdsatestvectors.zip
***************************************************************************** */
FIO_SFUNC void print_scalar(const char *name, const uint64_t s[4]) {
  fprintf(stderr, "%s: ", name);
  for (int i = 3; i >= 0; --i) {
    fprintf(stderr, "%016llx", (unsigned long long)s[i]);
  }
  fprintf(stderr, "\n");
}

FIO_SFUNC void FIO_NAME_TEST(stl, p256_ecdsa_nist)(void) {
  FIO_LOG_DDEBUG("Testing ECDSA P-256 with NIST test vectors");

  /* RFC 6979 A.2.5 P-256 SHA-256 test vector */
  /* Private key:
   * C9AFA9D845BA75166B5C215767B1D6934E50C3DB36E89B127B8A622B120F6721 */
  /* Public key:  Ux =
   * 60FED4BA255A9D31C961EB74C6356D68C049B8923B61FA6CE669622E60F29FB6 */
  /*              Uy =
   * 7903FE1008B8BC99A41AE9E95628BC64F2F1B20C2D7E9F5177A3C294D4462299 */
  /* Message: "sample" */
  /* clang-format off */

  /* SHA-256("sample") = af2bdbe1aa9b6ec1e2ade1d694f41fc71a831d0268e9891562113d8a62add1bf */
  static const uint8_t msg_hash1[32] = {
    0xaf, 0x2b, 0xdb, 0xe1, 0xaa, 0x9b, 0x6e, 0xc1,
    0xe2, 0xad, 0xe1, 0xd6, 0x94, 0xf4, 0x1f, 0xc7,
    0x1a, 0x83, 0x1d, 0x02, 0x68, 0xe9, 0x89, 0x15,
    0x62, 0x11, 0x3d, 0x8a, 0x62, 0xad, 0xd1, 0xbf
  };

  /* Qx = 60FED4BA255A9D31C961EB74C6356D68C049B8923B61FA6CE669622E60F29FB6 */
  /* Qy = 7903FE1008B8BC99A41AE9E95628BC64F2F1B20C2D7E9F5177A3C294D4462299 */
  static const uint8_t pubkey1[65] = {
    0x04,
    0x60, 0xfe, 0xd4, 0xba, 0x25, 0x5a, 0x9d, 0x31,
    0xc9, 0x61, 0xeb, 0x74, 0xc6, 0x35, 0x6d, 0x68,
    0xc0, 0x49, 0xb8, 0x92, 0x3b, 0x61, 0xfa, 0x6c,
    0xe6, 0x69, 0x62, 0x2e, 0x60, 0xf2, 0x9f, 0xb6,
    0x79, 0x03, 0xfe, 0x10, 0x08, 0xb8, 0xbc, 0x99,
    0xa4, 0x1a, 0xe9, 0xe9, 0x56, 0x28, 0xbc, 0x64,
    0xf2, 0xf1, 0xb2, 0x0c, 0x2d, 0x7e, 0x9f, 0x51,
    0x77, 0xa3, 0xc2, 0x94, 0xd4, 0x46, 0x22, 0x99
  };

  /* R = EFD48B2AACB6A8FD1140DD9CD45E81D69D2C877B56AAF991C34D0EA84EAF3716 */
  /* S = F7CB1C942D657C41D436C7A1B6E29F65F3E900DBB9AFF4064DC4AB2F843ACDA8 */
  static const uint8_t r1[32] = {
    0xef, 0xd4, 0x8b, 0x2a, 0xac, 0xb6, 0xa8, 0xfd,
    0x11, 0x40, 0xdd, 0x9c, 0xd4, 0x5e, 0x81, 0xd6,
    0x9d, 0x2c, 0x87, 0x7b, 0x56, 0xaa, 0xf9, 0x91,
    0xc3, 0x4d, 0x0e, 0xa8, 0x4e, 0xaf, 0x37, 0x16
  };
  static const uint8_t s1[32] = {
    0xf7, 0xcb, 0x1c, 0x94, 0x2d, 0x65, 0x7c, 0x41,
    0xd4, 0x36, 0xc7, 0xa1, 0xb6, 0xe2, 0x9f, 0x65,
    0xf3, 0xe9, 0x00, 0xdb, 0xb9, 0xaf, 0xf4, 0x06,
    0x4d, 0xc4, 0xab, 0x2f, 0x84, 0x3a, 0xcd, 0xa8
  };
  /* clang-format on */

  /* Debug: first test fio_math_div directly */
  FIO_LOG_DDEBUG("  Debug: testing fio_math_div...");
  {
    /* Test: 100 mod 7 = 2 */
    uint64_t a[4] = {100, 0, 0, 0};
    uint64_t b[4] = {7, 0, 0, 0};
    uint64_t q[4] = {0};
    uint64_t r[4] = {0};
    fio_math_div(q, r, a, b, 4);
    fprintf(stderr,
            "    100 / 7 = %llu, remainder = %llu\n",
            (unsigned long long)q[0],
            (unsigned long long)r[0]);
    FIO_ASSERT(q[0] == 14 && r[0] == 2, "100 / 7 should be 14 remainder 2");

    /* Test the specific 512-bit product that's failing */
    uint64_t product[8] = {0x3f5c31876a5f19c1ULL,  /* t[0] */
                           0x06702ff8fdd46558ULL,  /* t[1] */
                           0x559a010c2f72616aULL,  /* t[2] */
                           0x2abcad476384bb32ULL,  /* t[3] */
                           0x4b16c6ad97715bafULL,  /* t[4] */
                           0x974f16e7bcb01fd0ULL,  /* t[5] */
                           0x57c6b5a91829b255ULL,  /* t[6] */
                           0x003fb14f9b798fe4ULL}; /* t[7] */
    uint64_t n_ext2[8] = {FIO___P256_N[0],
                          FIO___P256_N[1],
                          FIO___P256_N[2],
                          FIO___P256_N[3],
                          0,
                          0,
                          0,
                          0};
    uint64_t rem2[8] = {0};
    fio_math_div(NULL, rem2, product, n_ext2, 8);
    fprintf(stderr,
            "    product mod n = %016llx%016llx%016llx%016llx\n",
            (unsigned long long)rem2[3],
            (unsigned long long)rem2[2],
            (unsigned long long)rem2[1],
            (unsigned long long)rem2[0]);
    fprintf(
        stderr,
        "    expected      = "
        "309f1aeeff274eaffb58ea09e2aad1f4e1d782283e63f349901e2513870a0f0e\n");

    /* Test: large number mod n */
    uint64_t large[8] = {0xFFFFFFFFFFFFFFFFULL,
                         0xFFFFFFFFFFFFFFFFULL,
                         0xFFFFFFFFFFFFFFFFULL,
                         0xFFFFFFFFFFFFFFFFULL,
                         0,
                         0,
                         0,
                         0};
    uint64_t n_ext[8] = {FIO___P256_N[0],
                         FIO___P256_N[1],
                         FIO___P256_N[2],
                         FIO___P256_N[3],
                         0,
                         0,
                         0,
                         0};
    uint64_t rem[8] = {0};
    fio_math_div(NULL, rem, large, n_ext, 8);
    print_scalar("(2^256-1) mod n", rem);
  }

  /* Debug: test scalar multiplication */
  FIO_LOG_DDEBUG("  Debug: testing scalar multiplication...");
  {
    fio___p256_scalar_s a = {7, 0, 0, 0};
    fio___p256_scalar_s b = {11, 0, 0, 0};
    fio___p256_scalar_s c;
    fio___p256_scalar_mul(c, a, b);
    print_scalar("7 * 11 mod n", c);
    FIO_ASSERT(c[0] == 77 && c[1] == 0 && c[2] == 0 && c[3] == 0,
               "7 * 11 should be 77");

    /* Test squaring: 7^2 = 49 */
    fio___p256_scalar_mul(c, a, a);
    print_scalar("7^2 mod n", c);
    FIO_ASSERT(c[0] == 49 && c[1] == 0 && c[2] == 0 && c[3] == 0,
               "7^2 should be 49");

    /* Test in-place squaring */
    fio___p256_scalar_s base = {7, 0, 0, 0};
    fio___p256_scalar_mul(base, base, base);
    print_scalar("7^2 (in-place)", base);
    FIO_ASSERT(base[0] == 49 && base[1] == 0 && base[2] == 0 && base[3] == 0,
               "7^2 in-place should be 49");

    /* Test 7^4 */
    fio___p256_scalar_mul(base, base, base);
    print_scalar("7^4 (in-place)", base);
    FIO_ASSERT(base[0] == 2401 && base[1] == 0 && base[2] == 0 && base[3] == 0,
               "7^4 in-place should be 2401");

    /* Test aliased multiplication: t = t * base where t=1, base=7 */
    fio___p256_scalar_s t2 = {1, 0, 0, 0};
    fio___p256_scalar_s base3 = {7, 0, 0, 0};
    fio___p256_scalar_mul(t2, t2, base3);
    print_scalar("1*7 aliased", t2);
    FIO_ASSERT(t2[0] == 7 && t2[1] == 0 && t2[2] == 0 && t2[3] == 0,
               "1*7 aliased should be 7");

    /* Test: t = t * base where t=7, base=49 */
    fio___p256_scalar_s base4 = {49, 0, 0, 0};
    fio___p256_scalar_mul(t2, t2, base4);
    print_scalar("7*49 aliased", t2);
    FIO_ASSERT(t2[0] == 343 && t2[1] == 0 && t2[2] == 0 && t2[3] == 0,
               "7*49 aliased should be 343");

    /* Test the specific failing case: base7^2 mod n */
    fio___p256_scalar_s base7 = {0x06a1b9cca51f951fULL,
                                 0xd61db914694e29afULL,
                                 0x499c0bb223d7ef38ULL,
                                 0x07fb1375c98bd3e0ULL};
    fio___p256_scalar_s base8;
    fio___p256_scalar_mul(base8, base7, base7);
    print_scalar("base7", base7);
    print_scalar("base7^2 mod n", base8);
    /* Expected:
     * 0x309f1aeeff274eaffb58ea09e2aad1f4e1d782283e63f349901e2513870a0f0e */
    print_scalar("expected     ",
                 (uint64_t[]){0x901e2513870a0f0eULL,
                              0xe1d782283e63f349ULL,
                              0xfb58ea09e2aad1f4ULL,
                              0x309f1aeeff274eafULL});

    /* Test scalar inversion manually step by step */
    /* Expected:
     * 0x49249248db6db6dbb6db6db6db6db6db5a8b230d0b2b51dcd7ebf0c9fef7c185 */
    fprintf(stderr, "\t  Manual scalar inversion trace:\n");
    {
      fio___p256_scalar_s t = {1, 0, 0, 0};
      fio___p256_scalar_s base2 = {7, 0, 0, 0};
      fio___p256_scalar_s tmp;

      /* n-2 =
       * 0xFFFFFFFF00000000FFFFFFFFFFFFFFFFBCE6FAADA7179E84F3B9CAC2FC63254F */
      static const uint64_t nm2[4] = {
          0xF3B9CAC2FC63254FULL,
          0xBCE6FAADA7179E84ULL,
          0xFFFFFFFFFFFFFFFFULL,
          0xFFFFFFFF00000000ULL,
      };

      /* Just do first 8 bits */
      uint64_t bits = nm2[0];
      for (int j = 0; j < 8; ++j) {
        fprintf(stderr, "    bit %d = %d, base = ", j, (int)(bits & 1));
        print_scalar("", base2);
        if (bits & 1) {
          fio___p256_scalar_mul(tmp, t, base2);
          t[0] = tmp[0];
          t[1] = tmp[1];
          t[2] = tmp[2];
          t[3] = tmp[3];
          fprintf(stderr, "      t after mul = ");
          print_scalar("", t);
        }
        fio___p256_scalar_mul(tmp, base2, base2);
        base2[0] = tmp[0];
        base2[1] = tmp[1];
        base2[2] = tmp[2];
        base2[3] = tmp[3];
        bits >>= 1;
      }
      print_scalar("After 8 bits, t", t);
    }

    fio___p256_scalar_s inv;
    fio___p256_scalar_inv(inv, a);
    print_scalar("7^(-1) mod n", inv);
    print_scalar("expected    ",
                 (uint64_t[]){0xd7ebf0c9fef7c185ULL,
                              0x5a8b230d0b2b51dcULL,
                              0xb6db6db6db6db6dbULL,
                              0x49249248db6db6dbULL});
    fio___p256_scalar_mul(c, a, inv);
    print_scalar("7 * 7^(-1) mod n", c);
    FIO_ASSERT(c[0] == 1 && c[1] == 0 && c[2] == 0 && c[3] == 0,
               "7 * 7^(-1) should be 1");
  }

  /* Debug: manually trace the verification */
  FIO_LOG_DDEBUG("  Debug: tracing ECDSA verification...");

  fio___p256_scalar_s r, s, e;
  fio___p256_point_affine_s q;

  /* Load signature components */
  fio___p256_scalar_from_bytes(r, r1);
  fio___p256_scalar_from_bytes(s, s1);
  fio___p256_scalar_from_bytes(e, msg_hash1);

  print_scalar("r", r);
  print_scalar("s", s);
  print_scalar("e (hash)", e);

  /* Load public key */
  fio___p256_fe_from_bytes(q.x, pubkey1 + 1);
  fio___p256_fe_from_bytes(q.y, pubkey1 + 33);
  print_fe("Q.x", q.x);
  print_fe("Q.y", q.y);

  /* Compute w = s^(-1) mod n */
  fio___p256_scalar_s w;
  fio___p256_scalar_inv(w, s);
  print_scalar("w = s^(-1)", w);

  /* Verify: s * w should equal 1 */
  fio___p256_scalar_s check;
  fio___p256_scalar_mul(check, s, w);
  print_scalar("s * w (should be 1)", check);

  /* Compute u1 = e * w mod n */
  fio___p256_scalar_s u1;
  fio___p256_scalar_mul(u1, e, w);
  print_scalar("u1 = e*w", u1);

  /* Compute u2 = r * w mod n */
  fio___p256_scalar_s u2;
  fio___p256_scalar_mul(u2, r, w);
  print_scalar("u2 = r*w", u2);

  /* Test: verify 3*G matches known value */
  /* 3G.x = 5ecbe4d1a6330a44c8f7ef951d4bf165e6c6b721efada985fb41661bc6e7fd6c */
  /* 3G.y = 8734640c4998ff7e374b06ce1a64a2ecd82ab036384fb83d9a79b127a27d5032 */
  {
    fio___p256_scalar_s three = {3, 0, 0, 0};
    fio___p256_point_affine_s g_test;
    fio___p256_fe_copy(g_test.x, FIO___P256_GX);
    fio___p256_fe_copy(g_test.y, FIO___P256_GY);
    fio___p256_point_jacobian_s threeG;
    fio___p256_point_mul(&threeG, three, &g_test);
    fio___p256_point_affine_s threeG_aff;
    fio___p256_point_to_affine(&threeG_aff, &threeG);
    print_fe("3G.x (computed)", threeG_aff.x);
    print_fe("3G.y (computed)", threeG_aff.y);
    fprintf(
        stderr,
        "3G.x (expected): "
        "5ecbe4d1a6330a44c8f7ef951d4bf165e6c6b721efada985fb41661bc6e7fd6c\n");
    fprintf(
        stderr,
        "3G.y (expected): "
        "8734640c4998ff7e374b06ce1a64a2ecd82ab036384fb83d9a79b127a27d5032\n");
  }

  /* Test: verify 4*G, 5*G, 6*G, 7*G */
  /* 4G.x = e2534a3532d08fbba02dde659ee62bd0031fe2db785596ef509302446b030852 */
  /* 5G.x = 51590b7a515140d2d784c85608668fdfef8c82fd1f5be52421554a0dc3d033ed */
  /* 6G.x = b01a172a76a4602c92d3242cb897dde3024c740debb215b4c6b0aae93c2291a9 */
  /* 7G.x = 8e533b6fa0bf7b4625bb30667c01fb607ef9f8b8a80fef5b300628703187b2a3 */
  {
    fio___p256_point_affine_s g_test;
    fio___p256_fe_copy(g_test.x, FIO___P256_GX);
    fio___p256_fe_copy(g_test.y, FIO___P256_GY);

    for (uint64_t k = 4; k <= 7; k++) {
      fio___p256_scalar_s scalar = {k, 0, 0, 0};
      fio___p256_point_jacobian_s kG;
      fio___p256_point_mul(&kG, scalar, &g_test);
      fio___p256_point_affine_s kG_aff;
      fio___p256_point_to_affine(&kG_aff, &kG);
      fprintf(stderr, "%lluG.x: ", (unsigned long long)k);
      for (int i = 3; i >= 0; --i)
        fprintf(stderr, "%016llx", (unsigned long long)kG_aff.x[i]);
      fprintf(stderr, "\n");
    }
    fprintf(
        stderr,
        "4G.x (expected): "
        "e2534a3532d08fbba02dde659ee62bd0031fe2db785596ef509302446b030852\n");
    fprintf(
        stderr,
        "5G.x (expected): "
        "51590b7a515140d2d784c85608668fdfef8c82fd1f5be52421554a0dc3d033ed\n");
    fprintf(
        stderr,
        "6G.x (expected): "
        "b01a172a76a4602c92d3242cb897dde3024c740debb215b4c6b0aae93c2291a9\n");
    fprintf(
        stderr,
        "7G.x (expected): "
        "8e533b6fa0bf7b4625bb30667c01fb607ef9f8b8a80fef5b300628703187b2a3\n");
  }

  /* Test with a scalar that has bit 64 set: 2^64 * G */
  {
    fio___p256_point_affine_s g_test;
    fio___p256_fe_copy(g_test.x, FIO___P256_GX);
    fio___p256_fe_copy(g_test.y, FIO___P256_GY);

    fio___p256_scalar_s k = {0, 1, 0, 0}; /* 2^64 */

    /* Manual trace */
    fio___p256_point_jacobian_s kG;
    fio___p256_point_set_infinity(&kG);

    int start_bit = 64; /* Only bit 64 is set */
    fprintf(stderr, "Manual 2^64*G trace:\n");
    for (int i = start_bit; i >= 0; --i) {
      fio___p256_point_double(&kG, &kG);

      int limb = i / 64;
      int bit = i % 64;
      int bit_val = (k[limb] >> bit) & 1;

      if (i >= 60 || i <= 5) {
        fio___p256_point_affine_s tmp_aff;
        fio___p256_point_to_affine(&tmp_aff, &kG);
        fprintf(stderr,
                "  i=%d, limb=%d, bit=%d, val=%d, after double: x=%016llx...\n",
                i,
                limb,
                bit,
                bit_val,
                (unsigned long long)tmp_aff.x[3]);
      }

      if (bit_val) {
        fio___p256_point_add_mixed(&kG, &kG, &g_test);
        if (i >= 60 || i <= 5) {
          fio___p256_point_affine_s tmp_aff;
          fio___p256_point_to_affine(&tmp_aff, &kG);
          fprintf(stderr,
                  "           after add:    x=%016llx...\n",
                  (unsigned long long)tmp_aff.x[3]);
        }
      }
    }

    fio___p256_point_affine_s kG_aff;
    fio___p256_point_to_affine(&kG_aff, &kG);
    print_fe("2^64*G.x (manual)", kG_aff.x);

    /* Also compute using point_mul */
    fio___p256_point_jacobian_s kG2;
    fio___p256_point_mul(&kG2, k, &g_test);
    fio___p256_point_affine_s kG2_aff;
    fio___p256_point_to_affine(&kG2_aff, &kG2);
    print_fe("2^64*G.x (point_mul)", kG2_aff.x);

    /* Verify it's on curve */
    fio___p256_fe_s y2, x3, t;
    fio___p256_fe_sqr(y2, kG_aff.y);
    fio___p256_fe_sqr(t, kG_aff.x);
    fio___p256_fe_mul(x3, t, kG_aff.x);
    fio___p256_fe_sub(t, x3, kG_aff.x);
    fio___p256_fe_sub(t, t, kG_aff.x);
    fio___p256_fe_sub(t, t, kG_aff.x);
    fio___p256_fe_add(t, t, FIO___P256_B);
    fprintf(stderr,
            "2^64*G on curve: %s\n",
            fio___p256_fe_eq(y2, t) == 0 ? "YES" : "NO");

    /* Compute 2^64*G by doubling G 64 times */
    fio___p256_point_jacobian_s doubled;
    fio___p256_point_to_jacobian(&doubled, &g_test);
    for (int i = 0; i < 64; i++) {
      fio___p256_point_double(&doubled, &doubled);
    }
    fio___p256_point_affine_s doubled_aff;
    fio___p256_point_to_affine(&doubled_aff, &doubled);
    print_fe("2^64*G.x (64 doubles)", doubled_aff.x);

    /* Check if they match */
    if (fio___p256_fe_eq(kG_aff.x, doubled_aff.x) == 0) {
      fprintf(stderr, "2^64*G: scalar mul matches 64 doubles\n");
    } else {
      fprintf(stderr, "2^64*G: MISMATCH between scalar mul and 64 doubles!\n");
    }
  }

  /* Test point addition: G + G should equal 2G */
  {
    fio___p256_point_affine_s g_aff2;
    fio___p256_fe_copy(g_aff2.x, FIO___P256_GX);
    fio___p256_fe_copy(g_aff2.y, FIO___P256_GY);

    /* Compute 2G via doubling */
    fio___p256_point_jacobian_s twoG_double;
    fio___p256_point_to_jacobian(&twoG_double, &g_aff2);
    fio___p256_point_double(&twoG_double, &twoG_double);
    fio___p256_point_affine_s twoG_double_aff;
    fio___p256_point_to_affine(&twoG_double_aff, &twoG_double);

    /* Compute G + G via addition (should trigger doubling internally) */
    fio___p256_point_jacobian_s twoG_add;
    fio___p256_point_to_jacobian(&twoG_add, &g_aff2);
    fio___p256_point_add_mixed(&twoG_add, &twoG_add, &g_aff2);
    fio___p256_point_affine_s twoG_add_aff;
    fio___p256_point_to_affine(&twoG_add_aff, &twoG_add);

    print_fe("2G (double).x", twoG_double_aff.x);
    print_fe("2G (add G+G).x", twoG_add_aff.x);

    if (fio___p256_fe_eq(twoG_double_aff.x, twoG_add_aff.x) == 0) {
      fprintf(stderr, "G + G = 2G: PASS\n");
    } else {
      fprintf(stderr, "G + G = 2G: FAIL\n");
    }
  }

  /* Test point addition: G + 2G should equal 3G */
  {
    fio___p256_point_affine_s g_aff2;
    fio___p256_fe_copy(g_aff2.x, FIO___P256_GX);
    fio___p256_fe_copy(g_aff2.y, FIO___P256_GY);

    /* Compute 2G */
    fio___p256_point_jacobian_s twoG;
    fio___p256_point_to_jacobian(&twoG, &g_aff2);
    fio___p256_point_double(&twoG, &twoG);
    fio___p256_point_affine_s twoG_aff;
    fio___p256_point_to_affine(&twoG_aff, &twoG);

    /* Compute 3G via scalar mul */
    fio___p256_scalar_s three = {3, 0, 0, 0};
    fio___p256_point_jacobian_s threeG_mul;
    fio___p256_point_mul(&threeG_mul, three, &g_aff2);
    fio___p256_point_affine_s threeG_mul_aff;
    fio___p256_point_to_affine(&threeG_mul_aff, &threeG_mul);

    /* Compute G + 2G via addition */
    fio___p256_point_jacobian_s threeG_add;
    fio___p256_point_to_jacobian(&threeG_add, &g_aff2);
    fio___p256_point_add_mixed(&threeG_add, &threeG_add, &twoG_aff);
    fio___p256_point_affine_s threeG_add_aff;
    fio___p256_point_to_affine(&threeG_add_aff, &threeG_add);

    print_fe("3G (scalar mul).x", threeG_mul_aff.x);
    print_fe("3G (G + 2G).x", threeG_add_aff.x);

    if (fio___p256_fe_eq(threeG_mul_aff.x, threeG_add_aff.x) == 0) {
      fprintf(stderr, "G + 2G = 3G: PASS\n");
    } else {
      fprintf(stderr, "G + 2G = 3G: FAIL\n");
    }
  }

  /* Test: verify d*G = Q using RFC 6979 A.2.5 test vector */
  /* d = c9afa9d845ba75166b5c215767b1d6934e50c3db36e89b127b8a622b120f6721 */
  /* Q.x = 60FED4BA255A9D31C961EB74C6356D68C049B8923B61FA6CE669622E60F29FB6 */
  /* Q.y = 7903FE1008B8BC99A41AE9E95628BC64F2F1B20C2D7E9F5177A3C294D4462299 */
  {
    static const uint8_t d_bytes[32] = {
        0xc9, 0xaf, 0xa9, 0xd8, 0x45, 0xba, 0x75, 0x16, 0x6b, 0x5c, 0x21,
        0x57, 0x67, 0xb1, 0xd6, 0x93, 0x4e, 0x50, 0xc3, 0xdb, 0x36, 0xe8,
        0x9b, 0x12, 0x7b, 0x8a, 0x62, 0x2b, 0x12, 0x0f, 0x67, 0x21};
    /* Expected public key from RFC 6979 */
    static const uint8_t expected_qx[32] = {
        0x60, 0xfe, 0xd4, 0xba, 0x25, 0x5a, 0x9d, 0x31, 0xc9, 0x61, 0xeb,
        0x74, 0xc6, 0x35, 0x6d, 0x68, 0xc0, 0x49, 0xb8, 0x92, 0x3b, 0x61,
        0xfa, 0x6c, 0xe6, 0x69, 0x62, 0x2e, 0x60, 0xf2, 0x9f, 0xb6};
    static const uint8_t expected_qy[32] = {
        0x79, 0x03, 0xfe, 0x10, 0x08, 0xb8, 0xbc, 0x99, 0xa4, 0x1a, 0xe9,
        0xe9, 0x56, 0x28, 0xbc, 0x64, 0xf2, 0xf1, 0xb2, 0x0c, 0x2d, 0x7e,
        0x9f, 0x51, 0x77, 0xa3, 0xc2, 0x94, 0xd4, 0x46, 0x22, 0x99};

    fio___p256_scalar_s d;
    fio___p256_scalar_from_bytes(d, d_bytes);
    print_scalar("d (private key)", d);

    /* Verify scalar loading */
    fprintf(stderr, "d_bytes: ");
    for (int i = 0; i < 32; i++)
      fprintf(stderr, "%02x", d_bytes[i]);
    fprintf(stderr, "\n");
    fprintf(stderr,
            "d limbs: [0]=%016llx [1]=%016llx [2]=%016llx [3]=%016llx\n",
            (unsigned long long)d[0],
            (unsigned long long)d[1],
            (unsigned long long)d[2],
            (unsigned long long)d[3]);

    fio___p256_point_affine_s g_test;
    fio___p256_fe_copy(g_test.x, FIO___P256_GX);
    fio___p256_fe_copy(g_test.y, FIO___P256_GY);

    /* Manually trace scalar multiplication for d */
    fio___p256_point_jacobian_s dG;
    fio___p256_point_set_infinity(&dG);

    /* Find highest set bit */
    int start_bit = 255;
    while (start_bit >= 0) {
      int limb = start_bit / 64;
      int bit = start_bit % 64;
      if (d[limb] & (1ULL << bit))
        break;
      --start_bit;
    }
    fprintf(stderr, "start_bit = %d\n", start_bit);

    /* Trace first 10 iterations */
    for (int i = start_bit; i >= 0; --i) {
      fio___p256_point_double(&dG, &dG);

      int limb = i / 64;
      int bit = i % 64;
      int bit_val = (d[limb] >> bit) & 1;

      if (i >= start_bit - 10) {
        fio___p256_point_affine_s tmp_aff;
        fio___p256_point_to_affine(&tmp_aff, &dG);
        fprintf(stderr,
                "i=%d, bit=%d, after double: x=%016llx...\n",
                i,
                bit_val,
                (unsigned long long)tmp_aff.x[3]);
      }

      if (bit_val) {
        fio___p256_point_add_mixed(&dG, &dG, &g_test);
        if (i >= start_bit - 10) {
          fio___p256_point_affine_s tmp_aff;
          fio___p256_point_to_affine(&tmp_aff, &dG);
          fprintf(stderr,
                  "         after add:    x=%016llx...\n",
                  (unsigned long long)tmp_aff.x[3]);
        }
      }
    }

    fio___p256_point_affine_s dG_aff;
    fio___p256_point_to_affine(&dG_aff, &dG);
    print_fe("d*G.x (computed)", dG_aff.x);
    print_fe("d*G.y (computed)", dG_aff.y);

    /* Load expected values from RFC 6979 */
    fio___p256_fe_s exp_qx, exp_qy;
    fio___p256_fe_from_bytes(exp_qx, expected_qx);
    fio___p256_fe_from_bytes(exp_qy, expected_qy);
    print_fe("Q.x (expected)  ", exp_qx);
    print_fe("Q.y (expected)  ", exp_qy);

    /* Verify d*G is on curve */
    {
      fio___p256_fe_s y2, x3, t;
      fio___p256_fe_sqr(y2, dG_aff.y);
      fio___p256_fe_sqr(t, dG_aff.x);
      fio___p256_fe_mul(x3, t, dG_aff.x);
      fio___p256_fe_sub(t, x3, dG_aff.x);
      fio___p256_fe_sub(t, t, dG_aff.x);
      fio___p256_fe_sub(t, t, dG_aff.x);
      fio___p256_fe_add(t, t, FIO___P256_B);
      fprintf(stderr,
              "d*G on curve: %s\n",
              fio___p256_fe_eq(y2, t) == 0 ? "YES" : "NO");
    }

    if (fio___p256_fe_eq(dG_aff.x, exp_qx) != 0 ||
        fio___p256_fe_eq(dG_aff.y, exp_qy) != 0) {
      fprintf(stderr, "ERROR: d*G != Q (RFC 6979)\n");
    } else {
      fprintf(stderr, "OK: d*G == Q (RFC 6979)\n");
    }
  }

  /* Debug: compute u1*G and u2*Q separately */
  fio___p256_point_affine_s g_aff;
  fio___p256_fe_copy(g_aff.x, FIO___P256_GX);
  fio___p256_fe_copy(g_aff.y, FIO___P256_GY);

  fio___p256_point_jacobian_s u1G_jac;
  fio___p256_point_mul(&u1G_jac, u1, &g_aff);
  fio___p256_point_affine_s u1G_aff;
  fio___p256_point_to_affine(&u1G_aff, &u1G_jac);
  print_fe("u1*G.x", u1G_aff.x);
  print_fe("u1*G.y", u1G_aff.y);

  /* Verify u1*G is on curve */
  {
    fio___p256_fe_s y2, x3, t;
    fio___p256_fe_sqr(y2, u1G_aff.y);
    fio___p256_fe_sqr(t, u1G_aff.x);
    fio___p256_fe_mul(x3, t, u1G_aff.x);
    fio___p256_fe_sub(t, x3, u1G_aff.x);
    fio___p256_fe_sub(t, t, u1G_aff.x);
    fio___p256_fe_sub(t, t, u1G_aff.x);
    fio___p256_fe_add(t, t, FIO___P256_B);
    fprintf(stderr,
            "u1*G on curve: %s\n",
            fio___p256_fe_eq(y2, t) == 0 ? "YES" : "NO");
  }

  fio___p256_point_jacobian_s u2Q_jac;
  fio___p256_point_mul(&u2Q_jac, u2, &q);
  fio___p256_point_affine_s u2Q_aff;
  fio___p256_point_to_affine(&u2Q_aff, &u2Q_jac);
  print_fe("u2*Q.x", u2Q_aff.x);
  print_fe("u2*Q.y", u2Q_aff.y);

  /* Verify u2*Q is on curve */
  {
    fio___p256_fe_s y2, x3, t;
    fio___p256_fe_sqr(y2, u2Q_aff.y);
    fio___p256_fe_sqr(t, u2Q_aff.x);
    fio___p256_fe_mul(x3, t, u2Q_aff.x);
    fio___p256_fe_sub(t, x3, u2Q_aff.x);
    fio___p256_fe_sub(t, t, u2Q_aff.x);
    fio___p256_fe_sub(t, t, u2Q_aff.x);
    fio___p256_fe_add(t, t, FIO___P256_B);
    fprintf(stderr,
            "u2*Q on curve: %s\n",
            fio___p256_fe_eq(y2, t) == 0 ? "YES" : "NO");
  }

  /* Compute u1*G + u2*Q manually */
  fio___p256_point_jacobian_s manual_R;
  fio___p256_point_to_jacobian(&manual_R, &u1G_aff);
  fio___p256_point_add_mixed(&manual_R, &manual_R, &u2Q_aff);
  fio___p256_point_affine_s manual_R_aff;
  fio___p256_point_to_affine(&manual_R_aff, &manual_R);
  print_fe("R.x (manual)", manual_R_aff.x);
  print_fe("R.y (manual)", manual_R_aff.y);

  /* Compute R = u1*G + u2*Q */
  fio___p256_point_jacobian_s R_jac;
  fio___p256_point_mul2(&R_jac, u1, u2, &q);

  /* Convert R to affine */
  fio___p256_point_affine_s R_aff;
  fio___p256_point_to_affine(&R_aff, &R_jac);
  print_fe("R.x", R_aff.x);
  print_fe("R.y", R_aff.y);

  /* Get R.x as bytes and load as scalar */
  uint8_t rx_bytes[32];
  fio___p256_fe_to_bytes(rx_bytes, R_aff.x);
  fio___p256_scalar_s rx;
  fio___p256_scalar_from_bytes(rx, rx_bytes);
  print_scalar("R.x as scalar", rx);

  /* Reduce mod n if needed */
  while (fio___p256_scalar_gte_n(rx)) {
    uint64_t borrow = 0;
    rx[0] = fio_math_subc64(rx[0], FIO___P256_N[0], 0, &borrow);
    rx[1] = fio_math_subc64(rx[1], FIO___P256_N[1], borrow, &borrow);
    rx[2] = fio_math_subc64(rx[2], FIO___P256_N[2], borrow, &borrow);
    rx[3] = fio_math_subc64(rx[3], FIO___P256_N[3], borrow, &borrow);
  }
  print_scalar("R.x mod n", rx);
  print_scalar("r (expected)", r);

  /* Test with raw r,s values */
  int result =
      fio_ecdsa_p256_verify_raw(r1, s1, msg_hash1, pubkey1 + 1, pubkey1 + 33);
  FIO_ASSERT(result == 0, "NIST ECDSA P-256 test vector 1 failed");

  FIO_LOG_DDEBUG("  NIST test vectors passed.");
}

/* *****************************************************************************
Test DER signature parsing
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, p256_der_parsing)(void) {
  FIO_LOG_DDEBUG("Testing DER signature parsing");

  /* Create a DER-encoded signature from RFC 6979 test vector */
  /* R = EFD48B2AACB6A8FD1140DD9CD45E81D69D2C877B56AAF991C34D0EA84EAF3716 */
  /* S = F7CB1C942D657C41D436C7A1B6E29F65F3E900DBB9AFF4064DC4AB2F843ACDA8 */

  /* clang-format off */
  static const uint8_t der_sig[] = {
    0x30, 0x45, /* SEQUENCE, length 69 */
    0x02, 0x20, /* INTEGER, length 32 */
    0xef, 0xd4, 0x8b, 0x2a, 0xac, 0xb6, 0xa8, 0xfd,
    0x11, 0x40, 0xdd, 0x9c, 0xd4, 0x5e, 0x81, 0xd6,
    0x9d, 0x2c, 0x87, 0x7b, 0x56, 0xaa, 0xf9, 0x91,
    0xc3, 0x4d, 0x0e, 0xa8, 0x4e, 0xaf, 0x37, 0x16,
    0x02, 0x21, /* INTEGER, length 33 */
    0x00, /* Leading zero (S >= 0x80) */
    0xf7, 0xcb, 0x1c, 0x94, 0x2d, 0x65, 0x7c, 0x41,
    0xd4, 0x36, 0xc7, 0xa1, 0xb6, 0xe2, 0x9f, 0x65,
    0xf3, 0xe9, 0x00, 0xdb, 0xb9, 0xaf, 0xf4, 0x06,
    0x4d, 0xc4, 0xab, 0x2f, 0x84, 0x3a, 0xcd, 0xa8
  };

  /* SHA-256("sample") = af2bdbe1aa9b6ec1e2ade1d694f41fc71a831d0268e9891562113d8a62add1bf */
  static const uint8_t msg_hash[32] = {
    0xaf, 0x2b, 0xdb, 0xe1, 0xaa, 0x9b, 0x6e, 0xc1,
    0xe2, 0xad, 0xe1, 0xd6, 0x94, 0xf4, 0x1f, 0xc7,
    0x1a, 0x83, 0x1d, 0x02, 0x68, 0xe9, 0x89, 0x15,
    0x62, 0x11, 0x3d, 0x8a, 0x62, 0xad, 0xd1, 0xbf
  };

  /* Qx = 60FED4BA255A9D31C961EB74C6356D68C049B8923B61FA6CE669622E60F29FB6 */
  /* Qy = 7903FE1008B8BC99A41AE9E95628BC64F2F1B20C2D7E9F5177A3C294D4462299 */
  static const uint8_t pubkey[65] = {
    0x04,
    0x60, 0xfe, 0xd4, 0xba, 0x25, 0x5a, 0x9d, 0x31,
    0xc9, 0x61, 0xeb, 0x74, 0xc6, 0x35, 0x6d, 0x68,
    0xc0, 0x49, 0xb8, 0x92, 0x3b, 0x61, 0xfa, 0x6c,
    0xe6, 0x69, 0x62, 0x2e, 0x60, 0xf2, 0x9f, 0xb6,
    0x79, 0x03, 0xfe, 0x10, 0x08, 0xb8, 0xbc, 0x99,
    0xa4, 0x1a, 0xe9, 0xe9, 0x56, 0x28, 0xbc, 0x64,
    0xf2, 0xf1, 0xb2, 0x0c, 0x2d, 0x7e, 0x9f, 0x51,
    0x77, 0xa3, 0xc2, 0x94, 0xd4, 0x46, 0x22, 0x99
  };
  /* clang-format on */

  int result =
      fio_ecdsa_p256_verify(der_sig, sizeof(der_sig), msg_hash, pubkey, 65);
  FIO_ASSERT(result == 0, "DER signature verification failed");

  FIO_LOG_DDEBUG("  DER parsing tests passed.");
}

/* *****************************************************************************
Test invalid signatures
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, p256_invalid_sigs)(void) {
  FIO_LOG_DDEBUG("Testing ECDSA P-256 rejects invalid signatures");

  /* Use RFC 6979 test vector for consistent testing */
  /* clang-format off */

  /* SHA-256("sample") */
  static const uint8_t msg_hash[32] = {
    0xaf, 0x2b, 0xdb, 0xe1, 0xaa, 0x9b, 0x6e, 0xc1,
    0xe2, 0xad, 0xe1, 0xd6, 0x94, 0xf4, 0x1f, 0xc7,
    0x1a, 0x83, 0x1d, 0x02, 0x68, 0xe9, 0x89, 0x15,
    0x62, 0x11, 0x3d, 0x8a, 0x62, 0xad, 0xd1, 0xbf
  };

  /* Qx = 60FED4BA..., Qy = 7903FE10... */
  static const uint8_t pubkey[65] = {
    0x04,
    0x60, 0xfe, 0xd4, 0xba, 0x25, 0x5a, 0x9d, 0x31,
    0xc9, 0x61, 0xeb, 0x74, 0xc6, 0x35, 0x6d, 0x68,
    0xc0, 0x49, 0xb8, 0x92, 0x3b, 0x61, 0xfa, 0x6c,
    0xe6, 0x69, 0x62, 0x2e, 0x60, 0xf2, 0x9f, 0xb6,
    0x79, 0x03, 0xfe, 0x10, 0x08, 0xb8, 0xbc, 0x99,
    0xa4, 0x1a, 0xe9, 0xe9, 0x56, 0x28, 0xbc, 0x64,
    0xf2, 0xf1, 0xb2, 0x0c, 0x2d, 0x7e, 0x9f, 0x51,
    0x77, 0xa3, 0xc2, 0x94, 0xd4, 0x46, 0x22, 0x99
  };

  /* R = EFD48B2AACB6A8FD1140DD9CD45E81D69D2C877B56AAF991C34D0EA84EAF3716 */
  static const uint8_t r[32] = {
    0xef, 0xd4, 0x8b, 0x2a, 0xac, 0xb6, 0xa8, 0xfd,
    0x11, 0x40, 0xdd, 0x9c, 0xd4, 0x5e, 0x81, 0xd6,
    0x9d, 0x2c, 0x87, 0x7b, 0x56, 0xaa, 0xf9, 0x91,
    0xc3, 0x4d, 0x0e, 0xa8, 0x4e, 0xaf, 0x37, 0x16
  };

  /* S = F7CB1C942D657C41D436C7A1B6E29F65F3E900DBB9AFF4064DC4AB2F843ACDA8 */
  static const uint8_t s[32] = {
    0xf7, 0xcb, 0x1c, 0x94, 0x2d, 0x65, 0x7c, 0x41,
    0xd4, 0x36, 0xc7, 0xa1, 0xb6, 0xe2, 0x9f, 0x65,
    0xf3, 0xe9, 0x00, 0xdb, 0xb9, 0xaf, 0xf4, 0x06,
    0x4d, 0xc4, 0xab, 0x2f, 0x84, 0x3a, 0xcd, 0xa8
  };
  /* clang-format on */

  /* First verify the valid signature works */
  int result =
      fio_ecdsa_p256_verify_raw(r, s, msg_hash, pubkey + 1, pubkey + 33);
  FIO_ASSERT(result == 0, "Valid signature should verify");

  /* Test 1: Modified r should fail */
  uint8_t bad_r[32];
  FIO_MEMCPY(bad_r, r, 32);
  bad_r[0] ^= 0x01;
  result =
      fio_ecdsa_p256_verify_raw(bad_r, s, msg_hash, pubkey + 1, pubkey + 33);
  FIO_ASSERT(result != 0, "Should reject modified r");

  /* Test 2: Modified s should fail */
  uint8_t bad_s[32];
  FIO_MEMCPY(bad_s, s, 32);
  bad_s[0] ^= 0x01;
  result =
      fio_ecdsa_p256_verify_raw(r, bad_s, msg_hash, pubkey + 1, pubkey + 33);
  FIO_ASSERT(result != 0, "Should reject modified s");

  /* Test 3: Modified message hash should fail */
  uint8_t bad_hash[32];
  FIO_MEMCPY(bad_hash, msg_hash, 32);
  bad_hash[0] ^= 0x01;
  result = fio_ecdsa_p256_verify_raw(r, s, bad_hash, pubkey + 1, pubkey + 33);
  FIO_ASSERT(result != 0, "Should reject modified message hash");

  /* Test 4: r = 0 should fail */
  uint8_t zero[32] = {0};
  result =
      fio_ecdsa_p256_verify_raw(zero, s, msg_hash, pubkey + 1, pubkey + 33);
  FIO_ASSERT(result != 0, "Should reject r = 0");

  /* Test 5: s = 0 should fail */
  result =
      fio_ecdsa_p256_verify_raw(r, zero, msg_hash, pubkey + 1, pubkey + 33);
  FIO_ASSERT(result != 0, "Should reject s = 0");

  FIO_LOG_DDEBUG("  Invalid signature tests passed.");
}

/* *****************************************************************************
Performance test
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, p256_performance)(void) {
#ifdef DEBUG
  FIO_LOG_DDEBUG("Skipping performance test in DEBUG mode");
  return;
#endif

  FIO_LOG_DDEBUG("Testing ECDSA P-256 performance");

  /* Use RFC 6979 test vector */
  /* clang-format off */

  /* SHA-256("sample") */
  static const uint8_t msg_hash[32] = {
    0xaf, 0x2b, 0xdb, 0xe1, 0xaa, 0x9b, 0x6e, 0xc1,
    0xe2, 0xad, 0xe1, 0xd6, 0x94, 0xf4, 0x1f, 0xc7,
    0x1a, 0x83, 0x1d, 0x02, 0x68, 0xe9, 0x89, 0x15,
    0x62, 0x11, 0x3d, 0x8a, 0x62, 0xad, 0xd1, 0xbf
  };

  /* Qx = 60FED4BA..., Qy = 7903FE10... */
  static const uint8_t pubkey[65] = {
    0x04,
    0x60, 0xfe, 0xd4, 0xba, 0x25, 0x5a, 0x9d, 0x31,
    0xc9, 0x61, 0xeb, 0x74, 0xc6, 0x35, 0x6d, 0x68,
    0xc0, 0x49, 0xb8, 0x92, 0x3b, 0x61, 0xfa, 0x6c,
    0xe6, 0x69, 0x62, 0x2e, 0x60, 0xf2, 0x9f, 0xb6,
    0x79, 0x03, 0xfe, 0x10, 0x08, 0xb8, 0xbc, 0x99,
    0xa4, 0x1a, 0xe9, 0xe9, 0x56, 0x28, 0xbc, 0x64,
    0xf2, 0xf1, 0xb2, 0x0c, 0x2d, 0x7e, 0x9f, 0x51,
    0x77, 0xa3, 0xc2, 0x94, 0xd4, 0x46, 0x22, 0x99
  };

  /* R = EFD48B2A... */
  static const uint8_t r[32] = {
    0xef, 0xd4, 0x8b, 0x2a, 0xac, 0xb6, 0xa8, 0xfd,
    0x11, 0x40, 0xdd, 0x9c, 0xd4, 0x5e, 0x81, 0xd6,
    0x9d, 0x2c, 0x87, 0x7b, 0x56, 0xaa, 0xf9, 0x91,
    0xc3, 0x4d, 0x0e, 0xa8, 0x4e, 0xaf, 0x37, 0x16
  };

  /* S = F7CB1C94... */
  static const uint8_t s[32] = {
    0xf7, 0xcb, 0x1c, 0x94, 0x2d, 0x65, 0x7c, 0x41,
    0xd4, 0x36, 0xc7, 0xa1, 0xb6, 0xe2, 0x9f, 0x65,
    0xf3, 0xe9, 0x00, 0xdb, 0xb9, 0xaf, 0xf4, 0x06,
    0x4d, 0xc4, 0xab, 0x2f, 0x84, 0x3a, 0xcd, 0xa8
  };
  /* clang-format on */

  const size_t iterations = 100;
  struct timespec start, end;

  clock_gettime(CLOCK_MONOTONIC, &start);
  for (size_t i = 0; i < iterations; ++i) {
    volatile int result =
        fio_ecdsa_p256_verify_raw(r, s, msg_hash, pubkey + 1, pubkey + 33);
    (void)result;
    FIO_COMPILER_GUARD;
  }
  clock_gettime(CLOCK_MONOTONIC, &end);

  double elapsed_us = (double)(end.tv_sec - start.tv_sec) * 1000000.0 +
                      (double)(end.tv_nsec - start.tv_nsec) / 1000.0;
  double ops_per_sec = (iterations * 1000000.0) / elapsed_us;

  fprintf(stderr,
          "\t  ECDSA P-256 verify: %.2f ops/sec (%.2f us/op)\n",
          ops_per_sec,
          elapsed_us / iterations);
}

/* *****************************************************************************
Test P-256 ECDHE Key Exchange
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, p256_ecdhe_keypair)(void) {
  FIO_LOG_DDEBUG("Testing P-256 ECDHE keypair generation");

  uint8_t sk1[32], pk1[65];
  uint8_t sk2[32], pk2[65];

  /* Generate first keypair */
  int result = fio_p256_keypair(sk1, pk1);
  FIO_ASSERT(result == 0, "First keypair generation failed");
  FIO_ASSERT(pk1[0] == 0x04, "Public key should be uncompressed (0x04 prefix)");

  /* Generate second keypair */
  result = fio_p256_keypair(sk2, pk2);
  FIO_ASSERT(result == 0, "Second keypair generation failed");
  FIO_ASSERT(pk2[0] == 0x04, "Public key should be uncompressed (0x04 prefix)");

  /* Keys should be different */
  FIO_ASSERT(memcmp(sk1, sk2, 32) != 0, "Secret keys should be different");
  FIO_ASSERT(memcmp(pk1, pk2, 65) != 0, "Public keys should be different");

  /* Verify public key is on curve by using shared secret with self */
  uint8_t shared[32];
  result = fio_p256_shared_secret(shared, sk1, pk1, 65);
  FIO_ASSERT(result == 0, "Self-shared secret computation should succeed");

  FIO_LOG_DDEBUG("  Keypair generation verified.");
}

FIO_SFUNC void FIO_NAME_TEST(stl, p256_ecdhe_shared_secret)(void) {
  FIO_LOG_DDEBUG("Testing P-256 ECDHE shared secret");

  uint8_t sk_alice[32], pk_alice[65];
  uint8_t sk_bob[32], pk_bob[65];
  uint8_t shared_alice[32], shared_bob[32];

  /* Generate keypairs */
  FIO_ASSERT(fio_p256_keypair(sk_alice, pk_alice) == 0,
             "Alice keypair generation failed");
  FIO_ASSERT(fio_p256_keypair(sk_bob, pk_bob) == 0,
             "Bob keypair generation failed");

  /* Compute shared secrets */
  int result = fio_p256_shared_secret(shared_alice, sk_alice, pk_bob, 65);
  FIO_ASSERT(result == 0, "Alice shared secret computation failed");

  result = fio_p256_shared_secret(shared_bob, sk_bob, pk_alice, 65);
  FIO_ASSERT(result == 0, "Bob shared secret computation failed");

  /* Shared secrets should be identical */
  FIO_ASSERT(memcmp(shared_alice, shared_bob, 32) == 0,
             "Alice and Bob shared secrets should match");

  /* Shared secret should not be all zeros */
  uint8_t zero_check = 0;
  for (int i = 0; i < 32; ++i)
    zero_check |= shared_alice[i];
  FIO_ASSERT(zero_check != 0, "Shared secret should not be all zeros");

  FIO_LOG_DDEBUG("  Shared secret exchange verified.");
}

FIO_SFUNC void FIO_NAME_TEST(stl, p256_ecdhe_invalid_inputs)(void) {
  FIO_LOG_DDEBUG("Testing P-256 ECDHE with invalid inputs");

  uint8_t sk[32], pk[65], shared[32];

  /* Generate a valid keypair */
  FIO_ASSERT(fio_p256_keypair(sk, pk) == 0, "Keypair generation failed");

  /* Test NULL parameters */
  FIO_ASSERT(fio_p256_keypair(NULL, pk) == -1, "NULL secret key should fail");
  FIO_ASSERT(fio_p256_keypair(sk, NULL) == -1, "NULL public key should fail");
  FIO_ASSERT(fio_p256_shared_secret(NULL, sk, pk, 65) == -1,
             "NULL shared secret should fail");
  FIO_ASSERT(fio_p256_shared_secret(shared, NULL, pk, 65) == -1,
             "NULL secret key should fail");
  FIO_ASSERT(fio_p256_shared_secret(shared, sk, NULL, 65) == -1,
             "NULL their public key should fail");

  /* Test invalid public key length */
  FIO_ASSERT(fio_p256_shared_secret(shared, sk, pk, 64) == -1,
             "Invalid key length (64) should fail");
  FIO_ASSERT(fio_p256_shared_secret(shared, sk, pk, 32) == -1,
             "Invalid key length (32) should fail");

  /* Test invalid public key prefix */
  uint8_t bad_pk[65];
  memcpy(bad_pk, pk, 65);
  bad_pk[0] = 0x05; /* Invalid prefix */
  FIO_ASSERT(fio_p256_shared_secret(shared, sk, bad_pk, 65) == -1,
             "Invalid prefix (0x05) should fail");

  /* Test point not on curve (corrupt y coordinate) */
  memcpy(bad_pk, pk, 65);
  bad_pk[64] ^= 0xFF; /* Flip bits in y coordinate */
  FIO_ASSERT(fio_p256_shared_secret(shared, sk, bad_pk, 65) == -1,
             "Point not on curve should fail");

  FIO_LOG_DDEBUG("  Invalid input handling verified.");
}

FIO_SFUNC void FIO_NAME_TEST(stl, p256_ecdhe_compressed)(void) {
  FIO_LOG_DDEBUG("Testing P-256 ECDHE with compressed public keys");

  uint8_t sk_alice[32], pk_alice[65];
  uint8_t sk_bob[32], pk_bob[65];
  uint8_t shared_uncomp[32], shared_comp[32];

  /* Generate keypairs */
  FIO_ASSERT(fio_p256_keypair(sk_alice, pk_alice) == 0,
             "Alice keypair generation failed");
  FIO_ASSERT(fio_p256_keypair(sk_bob, pk_bob) == 0,
             "Bob keypair generation failed");

  /* Compute shared secret with uncompressed key */
  FIO_ASSERT(fio_p256_shared_secret(shared_uncomp, sk_alice, pk_bob, 65) == 0,
             "Uncompressed shared secret failed");

  /* Create compressed version of Bob's public key */
  uint8_t pk_bob_compressed[33];
  pk_bob_compressed[0] = (pk_bob[64] & 1) ? 0x03 : 0x02;
  memcpy(pk_bob_compressed + 1, pk_bob + 1, 32);

  /* Compute shared secret with compressed key */
  int result =
      fio_p256_shared_secret(shared_comp, sk_alice, pk_bob_compressed, 33);
  FIO_ASSERT(result == 0, "Compressed shared secret failed");

  /* Both should produce the same result */
  FIO_ASSERT(memcmp(shared_uncomp, shared_comp, 32) == 0,
             "Compressed and uncompressed should give same shared secret");

  FIO_LOG_DDEBUG("  Compressed public key support verified.");
}

FIO_SFUNC void FIO_NAME_TEST(stl, p256_ecdhe_performance)(void) {
  FIO_LOG_DDEBUG("Testing P-256 ECDHE performance");

  uint8_t sk[32], pk[65], shared[32];
  const size_t iterations = 50;
  struct timespec start, end;

  /* Benchmark keypair generation */
  clock_gettime(CLOCK_MONOTONIC, &start);
  for (size_t i = 0; i < iterations; ++i) {
    volatile int result = fio_p256_keypair(sk, pk);
    (void)result;
    FIO_COMPILER_GUARD;
  }
  clock_gettime(CLOCK_MONOTONIC, &end);

  double elapsed_us = (double)(end.tv_sec - start.tv_sec) * 1000000.0 +
                      (double)(end.tv_nsec - start.tv_nsec) / 1000.0;
  double ops_per_sec = (iterations * 1000000.0) / elapsed_us;

  fprintf(stderr,
          "\t  P-256 keypair: %.2f ops/sec (%.2f us/op)\n",
          ops_per_sec,
          elapsed_us / iterations);

  /* Generate another keypair for shared secret test */
  uint8_t sk2[32], pk2[65];
  fio_p256_keypair(sk2, pk2);

  /* Benchmark shared secret computation */
  clock_gettime(CLOCK_MONOTONIC, &start);
  for (size_t i = 0; i < iterations; ++i) {
    volatile int result = fio_p256_shared_secret(shared, sk, pk2, 65);
    (void)result;
    FIO_COMPILER_GUARD;
  }
  clock_gettime(CLOCK_MONOTONIC, &end);

  elapsed_us = (double)(end.tv_sec - start.tv_sec) * 1000000.0 +
               (double)(end.tv_nsec - start.tv_nsec) / 1000.0;
  ops_per_sec = (iterations * 1000000.0) / elapsed_us;

  fprintf(stderr,
          "\t  P-256 shared secret: %.2f ops/sec (%.2f us/op)\n",
          ops_per_sec,
          elapsed_us / iterations);
}

/* *****************************************************************************
Main Test Function
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, p256)(void) {
  FIO_LOG_DDEBUG("Testing ECDSA P-256 (secp256r1) implementation");
  FIO_NAME_TEST(stl, p256_constants)();
  FIO_NAME_TEST(stl, p256_field_ops)();
  FIO_NAME_TEST(stl, p256_base_point)();
  FIO_NAME_TEST(stl, p256_scalar_mul)();
  FIO_NAME_TEST(stl, p256_ecdsa_nist)();
  FIO_NAME_TEST(stl, p256_der_parsing)();
  FIO_NAME_TEST(stl, p256_invalid_sigs)();
  FIO_NAME_TEST(stl, p256_performance)();

  FIO_LOG_DDEBUG("Testing P-256 ECDHE key exchange");
  FIO_NAME_TEST(stl, p256_ecdhe_keypair)();
  FIO_NAME_TEST(stl, p256_ecdhe_shared_secret)();
  FIO_NAME_TEST(stl, p256_ecdhe_invalid_inputs)();
  FIO_NAME_TEST(stl, p256_ecdhe_compressed)();
  FIO_NAME_TEST(stl, p256_ecdhe_performance)();

  FIO_LOG_DDEBUG("P-256 tests complete.");
}

/* *****************************************************************************
Test Runner
***************************************************************************** */

int main(int argc, char const *argv[]) {
  (void)argc;
  (void)argv;
  FIO_NAME_TEST(stl, p256)();
  return 0;
}
