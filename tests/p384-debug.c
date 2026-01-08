/* *****************************************************************************
P-384 Debug Tests - Isolate individual operations to find the bug
***************************************************************************** */
#include "test-helpers.h"

#define FIO_P384
#define FIO_SHA2
#define FIO_RAND
#define FIO_LOG
#include FIO_INCLUDE_FILE

/* *****************************************************************************
Helper functions
***************************************************************************** */
FIO_SFUNC void print_fe384(const char *name, const uint64_t fe[6]) {
  fprintf(stderr, "%s: ", name);
  for (int i = 5; i >= 0; --i) {
    fprintf(stderr, "%016llx", (unsigned long long)fe[i]);
  }
  fprintf(stderr, "\n");
}

FIO_SFUNC void print_bytes(const char *name, const uint8_t *data, size_t len) {
  fprintf(stderr, "%s: ", name);
  for (size_t i = 0; i < len; ++i) {
    fprintf(stderr, "%02x", data[i]);
  }
  fprintf(stderr, "\n");
}

FIO_SFUNC void print_point_affine(const char *name,
                                  const fio___p384_point_affine_s *p) {
  fprintf(stderr, "%s:\n", name);
  print_fe384("  x", p->x);
  print_fe384("  y", p->y);
}

FIO_SFUNC int point_on_curve(const fio___p384_point_affine_s *p) {
  fio___p384_fe_s y2, x3, t;
  fio___p384_fe_sqr(y2, p->y);
  fio___p384_fe_sqr(t, p->x);
  fio___p384_fe_mul(x3, t, p->x);
  fio___p384_fe_sub(t, x3, p->x);
  fio___p384_fe_sub(t, t, p->x);
  fio___p384_fe_sub(t, t, p->x);
  fio___p384_fe_add(t, t, FIO___P384_B);
  return fio___p384_fe_eq(y2, t) == 0;
}

/* *****************************************************************************
Test 1: Point Doubling (2*G)

Known result for 2*G on P-384:
2G.x = 08d999057ba3d2d969260045c55b97f089025959a6f434d651d207d19fb96e9e
       4fe0e86ebe0e64f85b96a9c75295df61
2G.y = 8e80f1fa5b1b3cedb7bfe8dffd6dba74b275d875bc6cc43e904e505f256ab425
       5ffd43e94d39e22d61501e700a940e80
***************************************************************************** */
FIO_SFUNC void test_point_doubling(void) {
  FIO_LOG_INFO("=== Test 1: Point Doubling (2*G) ===");

  /* Expected 2*G coordinates (from NIST/OpenSSL) */
  static const uint8_t expected_2g_x[48] = {
      0x08, 0xd9, 0x99, 0x05, 0x7b, 0xa3, 0xd2, 0xd9, 0x69, 0x26, 0x00, 0x45,
      0xc5, 0x5b, 0x97, 0xf0, 0x89, 0x02, 0x59, 0x59, 0xa6, 0xf4, 0x34, 0xd6,
      0x51, 0xd2, 0x07, 0xd1, 0x9f, 0xb9, 0x6e, 0x9e, 0x4f, 0xe0, 0xe8, 0x6e,
      0xbe, 0x0e, 0x64, 0xf8, 0x5b, 0x96, 0xa9, 0xc7, 0x52, 0x95, 0xdf, 0x61};
  static const uint8_t expected_2g_y[48] = {
      0x8e, 0x80, 0xf1, 0xfa, 0x5b, 0x1b, 0x3c, 0xed, 0xb7, 0xbf, 0xe8, 0xdf,
      0xfd, 0x6d, 0xba, 0x74, 0xb2, 0x75, 0xd8, 0x75, 0xbc, 0x6c, 0xc4, 0x3e,
      0x90, 0x4e, 0x50, 0x5f, 0x25, 0x6a, 0xb4, 0x25, 0x5f, 0xfd, 0x43, 0xe9,
      0x4d, 0x39, 0xe2, 0x2d, 0x61, 0x50, 0x1e, 0x70, 0x0a, 0x94, 0x0e, 0x80};

  fio___p384_fe_s expected_x, expected_y;
  fio___p384_fe_from_bytes(expected_x, expected_2g_x);
  fio___p384_fe_from_bytes(expected_y, expected_2g_y);

  /* Compute 2*G using point_double */
  fio___p384_point_affine_s g;
  fio___p384_fe_copy(g.x, FIO___P384_GX);
  fio___p384_fe_copy(g.y, FIO___P384_GY);

  fio___p384_point_jacobian_s g_jac, doubled;
  fio___p384_point_to_jacobian(&g_jac, &g);
  fio___p384_point_double(&doubled, &g_jac);

  fio___p384_point_affine_s result;
  fio___p384_point_to_affine(&result, &doubled);

  print_fe384("2G.x (computed)", result.x);
  print_fe384("2G.x (expected)", expected_x);
  print_fe384("2G.y (computed)", result.y);
  print_fe384("2G.y (expected)", expected_y);

  int x_match = (fio___p384_fe_eq(result.x, expected_x) == 0);
  int y_match = (fio___p384_fe_eq(result.y, expected_y) == 0);
  int on_curve = point_on_curve(&result);

  fprintf(stderr, "2G on curve: %s\n", on_curve ? "YES" : "NO");
  fprintf(stderr, "2G.x matches: %s\n", x_match ? "YES" : "NO");
  fprintf(stderr, "2G.y matches: %s\n", y_match ? "YES" : "NO");

  if (x_match && y_match) {
    FIO_LOG_INFO("  PASS: Point doubling is correct");
  } else {
    FIO_LOG_ERROR("  FAIL: Point doubling produces incorrect result");
  }
  fprintf(stderr, "\n");
}

/* *****************************************************************************
Test 2: Scalar Multiplication with small scalars (k*G)
***************************************************************************** */
FIO_SFUNC void test_scalar_mul_small(void) {
  FIO_LOG_INFO("=== Test 2: Scalar Multiplication with small scalars ===");

  fio___p384_point_affine_s g;
  fio___p384_fe_copy(g.x, FIO___P384_GX);
  fio___p384_fe_copy(g.y, FIO___P384_GY);

  /* Test 1*G = G */
  {
    fio___p384_scalar_s one = {1, 0, 0, 0, 0, 0};
    fio___p384_point_jacobian_s result_jac;
    fio___p384_point_mul(&result_jac, one, &g);

    fio___p384_point_affine_s result;
    fio___p384_point_to_affine(&result, &result_jac);

    int x_match = (fio___p384_fe_eq(result.x, FIO___P384_GX) == 0);
    int y_match = (fio___p384_fe_eq(result.y, FIO___P384_GY) == 0);

    fprintf(stderr, "1*G = G: %s\n", (x_match && y_match) ? "PASS" : "FAIL");
    if (!x_match || !y_match) {
      print_fe384("1*G.x (computed)", result.x);
      print_fe384("G.x   (expected)", FIO___P384_GX);
    }
  }

  /* Test 2*G via scalar multiplication */
  {
    static const uint8_t expected_2g_x[48] = {
        0x08, 0xd9, 0x99, 0x05, 0x7b, 0xa3, 0xd2, 0xd9, 0x69, 0x26, 0x00, 0x45,
        0xc5, 0x5b, 0x97, 0xf0, 0x89, 0x02, 0x59, 0x59, 0xa6, 0xf4, 0x34, 0xd6,
        0x51, 0xd2, 0x07, 0xd1, 0x9f, 0xb9, 0x6e, 0x9e, 0x4f, 0xe0, 0xe8, 0x6e,
        0xbe, 0x0e, 0x64, 0xf8, 0x5b, 0x96, 0xa9, 0xc7, 0x52, 0x95, 0xdf, 0x61};

    fio___p384_fe_s expected_x;
    fio___p384_fe_from_bytes(expected_x, expected_2g_x);

    fio___p384_scalar_s two = {2, 0, 0, 0, 0, 0};
    fio___p384_point_jacobian_s result_jac;
    fio___p384_point_mul(&result_jac, two, &g);

    fio___p384_point_affine_s result;
    fio___p384_point_to_affine(&result, &result_jac);

    int x_match = (fio___p384_fe_eq(result.x, expected_x) == 0);
    int on_curve = point_on_curve(&result);

    fprintf(stderr, "2*G on curve: %s\n", on_curve ? "YES" : "NO");
    fprintf(stderr, "2*G.x matches: %s\n", x_match ? "PASS" : "FAIL");
    if (!x_match) {
      print_fe384("2*G.x (computed)", result.x);
      print_fe384("2*G.x (expected)", expected_x);
    }
  }

  /* Test 3*G = 2*G + G */
  {
    /* 3G.x =
     * 077a41d4606ffa1464793c7e5fdc7d98cb9d3910202dcd06bea4f240d3566da6b408bbae5026580d02d7e5c70500c831
     */
    static const uint8_t expected_3g_x[48] = {
        0x07, 0x7a, 0x41, 0xd4, 0x60, 0x6f, 0xfa, 0x14, 0x64, 0x79, 0x3c, 0x7e,
        0x5f, 0xdc, 0x7d, 0x98, 0xcb, 0x9d, 0x39, 0x10, 0x20, 0x2d, 0xcd, 0x06,
        0xbe, 0xa4, 0xf2, 0x40, 0xd3, 0x56, 0x6d, 0xa6, 0xb4, 0x08, 0xbb, 0xae,
        0x50, 0x26, 0x58, 0x0d, 0x02, 0xd7, 0xe5, 0xc7, 0x05, 0x00, 0xc8, 0x31};

    fio___p384_fe_s expected_x;
    fio___p384_fe_from_bytes(expected_x, expected_3g_x);

    fio___p384_scalar_s three = {3, 0, 0, 0, 0, 0};
    fio___p384_point_jacobian_s result_jac;
    fio___p384_point_mul(&result_jac, three, &g);

    fio___p384_point_affine_s result;
    fio___p384_point_to_affine(&result, &result_jac);

    int x_match = (fio___p384_fe_eq(result.x, expected_x) == 0);
    int on_curve = point_on_curve(&result);

    fprintf(stderr, "3*G on curve: %s\n", on_curve ? "YES" : "NO");
    fprintf(stderr, "3*G.x matches: %s\n", x_match ? "PASS" : "FAIL");
    if (!x_match) {
      print_fe384("3*G.x (computed)", result.x);
      print_fe384("3*G.x (expected)", expected_x);
    }
  }

  fprintf(stderr, "\n");
}

/* *****************************************************************************
Test 3: Point Addition (G + 2G = 3G)
***************************************************************************** */
FIO_SFUNC void test_point_addition(void) {
  FIO_LOG_INFO("=== Test 3: Point Addition (G + 2G = 3G) ===");

  /* 3G.x =
   * 077a41d4606ffa1464793c7e5fdc7d98cb9d3910202dcd06bea4f240d3566da6b408bbae5026580d02d7e5c70500c831
   */
  static const uint8_t expected_3g_x[48] = {
      0x07, 0x7a, 0x41, 0xd4, 0x60, 0x6f, 0xfa, 0x14, 0x64, 0x79, 0x3c, 0x7e,
      0x5f, 0xdc, 0x7d, 0x98, 0xcb, 0x9d, 0x39, 0x10, 0x20, 0x2d, 0xcd, 0x06,
      0xbe, 0xa4, 0xf2, 0x40, 0xd3, 0x56, 0x6d, 0xa6, 0xb4, 0x08, 0xbb, 0xae,
      0x50, 0x26, 0x58, 0x0d, 0x02, 0xd7, 0xe5, 0xc7, 0x05, 0x00, 0xc8, 0x31};

  fio___p384_fe_s expected_x;
  fio___p384_fe_from_bytes(expected_x, expected_3g_x);

  /* Get G */
  fio___p384_point_affine_s g;
  fio___p384_fe_copy(g.x, FIO___P384_GX);
  fio___p384_fe_copy(g.y, FIO___P384_GY);

  /* Compute 2G */
  fio___p384_point_jacobian_s g_jac, two_g_jac;
  fio___p384_point_to_jacobian(&g_jac, &g);
  fio___p384_point_double(&two_g_jac, &g_jac);

  fio___p384_point_affine_s two_g;
  fio___p384_point_to_affine(&two_g, &two_g_jac);

  /* Compute G + 2G using mixed addition */
  fio___p384_point_jacobian_s three_g_jac;
  fio___p384_point_add_mixed(&three_g_jac, &two_g_jac, &g);

  fio___p384_point_affine_s three_g;
  fio___p384_point_to_affine(&three_g, &three_g_jac);

  int x_match = (fio___p384_fe_eq(three_g.x, expected_x) == 0);
  int on_curve = point_on_curve(&three_g);

  fprintf(stderr, "G + 2G on curve: %s\n", on_curve ? "YES" : "NO");
  fprintf(stderr, "G + 2G = 3G: %s\n", x_match ? "PASS" : "FAIL");
  if (!x_match) {
    print_fe384("(G+2G).x (computed)", three_g.x);
    print_fe384("3G.x     (expected)", expected_x);
  }

  fprintf(stderr, "\n");
}

/* *****************************************************************************
Test 4: Scalar multiplication with arbitrary point Q
Using the RFC 6979 test vector public key
***************************************************************************** */
FIO_SFUNC void test_scalar_mul_arbitrary(void) {
  FIO_LOG_INFO("=== Test 4: Scalar Multiplication with arbitrary point Q ===");

  /* Public key Q from RFC 6979 A.2.6 */
  static const uint8_t qx_bytes[48] = {
      0xec, 0x3a, 0x4e, 0x41, 0x5b, 0x4e, 0x19, 0xa4, 0x56, 0x86, 0x18, 0x02,
      0x9f, 0x42, 0x7f, 0xa5, 0xda, 0x9a, 0x8b, 0xc4, 0xae, 0x92, 0xe0, 0x2e,
      0x06, 0xaa, 0xe5, 0x28, 0x6b, 0x30, 0x0c, 0x64, 0xde, 0xf8, 0xf0, 0xea,
      0x90, 0x55, 0x86, 0x60, 0x64, 0xa2, 0x54, 0x51, 0x54, 0x80, 0xbc, 0x13};
  static const uint8_t qy_bytes[48] = {
      0x80, 0x15, 0xd9, 0xb7, 0x2d, 0x7d, 0x57, 0x24, 0x4e, 0xa8, 0xef, 0x9a,
      0xc0, 0xc6, 0x21, 0x89, 0x67, 0x08, 0xa5, 0x93, 0x67, 0xf9, 0xdf, 0xb9,
      0xf5, 0x4c, 0xa8, 0x4b, 0x3f, 0x1c, 0x9d, 0xb1, 0x28, 0x8b, 0x23, 0x1c,
      0x3a, 0xe0, 0xd4, 0xfe, 0x73, 0x44, 0xfd, 0x25, 0x33, 0x26, 0x47, 0x20};

  fio___p384_point_affine_s q;
  fio___p384_fe_from_bytes(q.x, qx_bytes);
  fio___p384_fe_from_bytes(q.y, qy_bytes);

  fprintf(stderr, "Q on curve: %s\n", point_on_curve(&q) ? "YES" : "NO");

  /* Test 1*Q = Q */
  {
    fio___p384_scalar_s one = {1, 0, 0, 0, 0, 0};
    fio___p384_point_jacobian_s result_jac;
    fio___p384_point_mul(&result_jac, one, &q);

    fio___p384_point_affine_s result;
    fio___p384_point_to_affine(&result, &result_jac);

    int x_match = (fio___p384_fe_eq(result.x, q.x) == 0);
    int y_match = (fio___p384_fe_eq(result.y, q.y) == 0);

    fprintf(stderr, "1*Q = Q: %s\n", (x_match && y_match) ? "PASS" : "FAIL");
    if (!x_match || !y_match) {
      print_fe384("1*Q.x (computed)", result.x);
      print_fe384("Q.x   (expected)", q.x);
    }
  }

  /* Test 2*Q */
  {
    fio___p384_scalar_s two = {2, 0, 0, 0, 0, 0};
    fio___p384_point_jacobian_s result_jac;
    fio___p384_point_mul(&result_jac, two, &q);

    fio___p384_point_affine_s result;
    fio___p384_point_to_affine(&result, &result_jac);

    int on_curve = point_on_curve(&result);
    fprintf(stderr, "2*Q on curve: %s\n", on_curve ? "YES" : "NO");
    print_fe384("2*Q.x", result.x);
  }

  fprintf(stderr, "\n");
}

/* *****************************************************************************
Test 5: Double scalar multiplication (u1*G + u2*Q)
This is the critical operation for ECDSA verification
***************************************************************************** */
FIO_SFUNC void test_double_scalar_mul(void) {
  FIO_LOG_INFO("=== Test 5: Double Scalar Multiplication (u1*G + u2*Q) ===");

  /* Use the RFC 6979 test vector values */
  /* clang-format off */
  static const uint8_t msg_hash[48] = {
    0xfd, 0xa5, 0x4e, 0x30, 0x04, 0x9d, 0x8a, 0xcd,
    0xd5, 0x1c, 0xa3, 0x35, 0x0b, 0x97, 0x8a, 0x2d,
    0xc8, 0x5d, 0x3d, 0x33, 0x03, 0xf9, 0x6d, 0x81,
    0x08, 0xd4, 0x2a, 0x47, 0x67, 0x99, 0xd6, 0x12,
    0x22, 0xc2, 0x4c, 0xb4, 0x84, 0xe3, 0x1b, 0xd8,
    0xf1, 0xec, 0xb4, 0x10, 0x49, 0x1e, 0xdc, 0x21
  };

  static const uint8_t qx_bytes[48] = {
    0xec, 0x3a, 0x4e, 0x41, 0x5b, 0x4e, 0x19, 0xa4,
    0x56, 0x86, 0x18, 0x02, 0x9f, 0x42, 0x7f, 0xa5,
    0xda, 0x9a, 0x8b, 0xc4, 0xae, 0x92, 0xe0, 0x2e,
    0x06, 0xaa, 0xe5, 0x28, 0x6b, 0x30, 0x0c, 0x64,
    0xde, 0xf8, 0xf0, 0xea, 0x90, 0x55, 0x86, 0x60,
    0x64, 0xa2, 0x54, 0x51, 0x54, 0x80, 0xbc, 0x13
  };
  static const uint8_t qy_bytes[48] = {
    0x80, 0x15, 0xd9, 0xb7, 0x2d, 0x7d, 0x57, 0x24,
    0x4e, 0xa8, 0xef, 0x9a, 0xc0, 0xc6, 0x21, 0x89,
    0x67, 0x08, 0xa5, 0x93, 0x67, 0xf9, 0xdf, 0xb9,
    0xf5, 0x4c, 0xa8, 0x4b, 0x3f, 0x1c, 0x9d, 0xb1,
    0x28, 0x8b, 0x23, 0x1c, 0x3a, 0xe0, 0xd4, 0xfe,
    0x73, 0x44, 0xfd, 0x25, 0x33, 0x26, 0x47, 0x20
  };

  static const uint8_t r_bytes[48] = {
    0x94, 0xed, 0xbb, 0x92, 0xa5, 0xec, 0xb8, 0xaa,
    0xd4, 0x73, 0x6e, 0x56, 0xc6, 0x91, 0x91, 0x6b,
    0x3f, 0x88, 0x14, 0x06, 0x66, 0xce, 0x9f, 0xa7,
    0x3d, 0x64, 0xc4, 0xea, 0x95, 0xad, 0x13, 0x3c,
    0x81, 0xa6, 0x48, 0x15, 0x2e, 0x44, 0xac, 0xf9,
    0x6e, 0x36, 0xdd, 0x1e, 0x80, 0xfa, 0xbe, 0x46
  };
  static const uint8_t s_bytes[48] = {
    0x99, 0xef, 0x4a, 0xeb, 0x15, 0xf1, 0x78, 0xce,
    0xa1, 0xfe, 0x40, 0xdb, 0x26, 0x03, 0x13, 0x8f,
    0x13, 0x0e, 0x74, 0x0a, 0x19, 0x62, 0x45, 0x26,
    0x20, 0x3b, 0x63, 0x51, 0xd0, 0xa3, 0xa9, 0x4f,
    0xa3, 0x29, 0xc1, 0x45, 0x78, 0x6e, 0x67, 0x9e,
    0x7b, 0x82, 0xc7, 0x1a, 0x38, 0x62, 0x8a, 0xc8
  };
  /* clang-format on */

  /* Load values */
  fio___p384_scalar_s r, s, e;
  fio___p384_point_affine_s q;

  fio___p384_scalar_from_bytes(r, r_bytes);
  fio___p384_scalar_from_bytes(s, s_bytes);
  fio___p384_scalar_from_bytes(e, msg_hash);
  fio___p384_fe_from_bytes(q.x, qx_bytes);
  fio___p384_fe_from_bytes(q.y, qy_bytes);

  /* Compute w = s^(-1) mod n */
  fio___p384_scalar_s w;
  fio___p384_scalar_inv(w, s);

  /* Compute u1 = e * w mod n */
  fio___p384_scalar_s u1;
  fio___p384_scalar_mul(u1, e, w);

  /* Compute u2 = r * w mod n */
  fio___p384_scalar_s u2;
  fio___p384_scalar_mul(u2, r, w);

  print_fe384("u1", u1);
  print_fe384("u2", u2);

  /* Method 1: Compute u1*G and u2*Q separately, then add */
  FIO_LOG_INFO("  Method 1: Separate scalar muls then add");
  {
    fio___p384_point_affine_s g;
    fio___p384_fe_copy(g.x, FIO___P384_GX);
    fio___p384_fe_copy(g.y, FIO___P384_GY);

    /* u1*G */
    fio___p384_point_jacobian_s u1g_jac;
    fio___p384_point_mul(&u1g_jac, u1, &g);
    fio___p384_point_affine_s u1g;
    fio___p384_point_to_affine(&u1g, &u1g_jac);

    fprintf(stderr, "u1*G on curve: %s\n", point_on_curve(&u1g) ? "YES" : "NO");
    print_fe384("u1*G.x", u1g.x);

    /* u2*Q */
    fio___p384_point_jacobian_s u2q_jac;
    fio___p384_point_mul(&u2q_jac, u2, &q);
    fio___p384_point_affine_s u2q;
    fio___p384_point_to_affine(&u2q, &u2q_jac);

    fprintf(stderr, "u2*Q on curve: %s\n", point_on_curve(&u2q) ? "YES" : "NO");
    print_fe384("u2*Q.x", u2q.x);

    /* Add u1*G + u2*Q */
    fio___p384_point_jacobian_s sum_jac;
    fio___p384_point_add_mixed(&sum_jac, &u1g_jac, &u2q);
    fio___p384_point_affine_s sum;
    fio___p384_point_to_affine(&sum, &sum_jac);

    fprintf(stderr,
            "u1*G + u2*Q on curve: %s\n",
            point_on_curve(&sum) ? "YES" : "NO");
    print_fe384("(u1*G + u2*Q).x (separate)", sum.x);

    /* Get R.x mod n */
    uint8_t rx_bytes[48];
    fio___p384_fe_to_bytes(rx_bytes, sum.x);
    fio___p384_scalar_s rx;
    fio___p384_scalar_from_bytes(rx, rx_bytes);
    while (fio___p384_scalar_gte_n(rx)) {
      uint64_t borrow = 0;
      rx[0] = fio_math_subc64(rx[0], FIO___P384_N[0], 0, &borrow);
      rx[1] = fio_math_subc64(rx[1], FIO___P384_N[1], borrow, &borrow);
      rx[2] = fio_math_subc64(rx[2], FIO___P384_N[2], borrow, &borrow);
      rx[3] = fio_math_subc64(rx[3], FIO___P384_N[3], borrow, &borrow);
      rx[4] = fio_math_subc64(rx[4], FIO___P384_N[4], borrow, &borrow);
      rx[5] = fio_math_subc64(rx[5], FIO___P384_N[5], borrow, &borrow);
    }
    print_fe384("R.x mod n (separate)", rx);
    print_fe384("r (expected)", r);

    int match = (rx[0] == r[0] && rx[1] == r[1] && rx[2] == r[2] &&
                 rx[3] == r[3] && rx[4] == r[4] && rx[5] == r[5]);
    fprintf(stderr, "Method 1 result matches r: %s\n", match ? "YES" : "NO");
  }

  /* Method 2: Use fio___p384_point_mul2 (Shamir's trick) */
  FIO_LOG_INFO("  Method 2: Using fio___p384_point_mul2 (Shamir's trick)");
  {
    fio___p384_point_jacobian_s R_jac;
    fio___p384_point_mul2(&R_jac, u1, u2, &q);

    fio___p384_point_affine_s R;
    fio___p384_point_to_affine(&R, &R_jac);

    fprintf(stderr,
            "point_mul2 result on curve: %s\n",
            point_on_curve(&R) ? "YES" : "NO");
    print_fe384("(u1*G + u2*Q).x (mul2)", R.x);

    /* Get R.x mod n */
    uint8_t rx_bytes[48];
    fio___p384_fe_to_bytes(rx_bytes, R.x);
    fio___p384_scalar_s rx;
    fio___p384_scalar_from_bytes(rx, rx_bytes);
    while (fio___p384_scalar_gte_n(rx)) {
      uint64_t borrow = 0;
      rx[0] = fio_math_subc64(rx[0], FIO___P384_N[0], 0, &borrow);
      rx[1] = fio_math_subc64(rx[1], FIO___P384_N[1], borrow, &borrow);
      rx[2] = fio_math_subc64(rx[2], FIO___P384_N[2], borrow, &borrow);
      rx[3] = fio_math_subc64(rx[3], FIO___P384_N[3], borrow, &borrow);
      rx[4] = fio_math_subc64(rx[4], FIO___P384_N[4], borrow, &borrow);
      rx[5] = fio_math_subc64(rx[5], FIO___P384_N[5], borrow, &borrow);
    }
    print_fe384("R.x mod n (mul2)", rx);
    print_fe384("r (expected)", r);

    int match = (rx[0] == r[0] && rx[1] == r[1] && rx[2] == r[2] &&
                 rx[3] == r[3] && rx[4] == r[4] && rx[5] == r[5]);
    fprintf(stderr, "Method 2 result matches r: %s\n", match ? "YES" : "NO");
  }

  fprintf(stderr, "\n");
}

/* *****************************************************************************
Test 6: Simple test of G+Q precomputation in point_mul2
***************************************************************************** */
FIO_SFUNC void test_gpq_precomputation(void) {
  FIO_LOG_INFO("=== Test 6: G+Q Precomputation ===");

  /* Use the RFC 6979 test vector public key Q */
  static const uint8_t qx_bytes[48] = {
      0xec, 0x3a, 0x4e, 0x41, 0x5b, 0x4e, 0x19, 0xa4, 0x56, 0x86, 0x18, 0x02,
      0x9f, 0x42, 0x7f, 0xa5, 0xda, 0x9a, 0x8b, 0xc4, 0xae, 0x92, 0xe0, 0x2e,
      0x06, 0xaa, 0xe5, 0x28, 0x6b, 0x30, 0x0c, 0x64, 0xde, 0xf8, 0xf0, 0xea,
      0x90, 0x55, 0x86, 0x60, 0x64, 0xa2, 0x54, 0x51, 0x54, 0x80, 0xbc, 0x13};
  static const uint8_t qy_bytes[48] = {
      0x80, 0x15, 0xd9, 0xb7, 0x2d, 0x7d, 0x57, 0x24, 0x4e, 0xa8, 0xef, 0x9a,
      0xc0, 0xc6, 0x21, 0x89, 0x67, 0x08, 0xa5, 0x93, 0x67, 0xf9, 0xdf, 0xb9,
      0xf5, 0x4c, 0xa8, 0x4b, 0x3f, 0x1c, 0x9d, 0xb1, 0x28, 0x8b, 0x23, 0x1c,
      0x3a, 0xe0, 0xd4, 0xfe, 0x73, 0x44, 0xfd, 0x25, 0x33, 0x26, 0x47, 0x20};

  fio___p384_point_affine_s q;
  fio___p384_fe_from_bytes(q.x, qx_bytes);
  fio___p384_fe_from_bytes(q.y, qy_bytes);

  fio___p384_point_affine_s g;
  fio___p384_fe_copy(g.x, FIO___P384_GX);
  fio___p384_fe_copy(g.y, FIO___P384_GY);

  /* Compute G + Q */
  fio___p384_point_jacobian_s g_jac;
  fio___p384_point_to_jacobian(&g_jac, &g);
  fio___p384_point_add_mixed(&g_jac, &g_jac, &q);

  fio___p384_point_affine_s gpq;
  fio___p384_point_to_affine(&gpq, &g_jac);

  fprintf(stderr, "G on curve: %s\n", point_on_curve(&g) ? "YES" : "NO");
  fprintf(stderr, "Q on curve: %s\n", point_on_curve(&q) ? "YES" : "NO");
  fprintf(stderr, "G+Q on curve: %s\n", point_on_curve(&gpq) ? "YES" : "NO");

  print_point_affine("G", &g);
  print_point_affine("Q", &q);
  print_point_affine("G+Q", &gpq);

  fprintf(stderr, "\n");
}

/* *****************************************************************************
Test 7: Test with simple scalars u1=1, u2=0 (should give G)
and u1=0, u2=1 (should give Q)
***************************************************************************** */
FIO_SFUNC void test_mul2_simple_cases(void) {
  FIO_LOG_INFO("=== Test 7: point_mul2 with simple scalars ===");

  static const uint8_t qx_bytes[48] = {
      0xec, 0x3a, 0x4e, 0x41, 0x5b, 0x4e, 0x19, 0xa4, 0x56, 0x86, 0x18, 0x02,
      0x9f, 0x42, 0x7f, 0xa5, 0xda, 0x9a, 0x8b, 0xc4, 0xae, 0x92, 0xe0, 0x2e,
      0x06, 0xaa, 0xe5, 0x28, 0x6b, 0x30, 0x0c, 0x64, 0xde, 0xf8, 0xf0, 0xea,
      0x90, 0x55, 0x86, 0x60, 0x64, 0xa2, 0x54, 0x51, 0x54, 0x80, 0xbc, 0x13};
  static const uint8_t qy_bytes[48] = {
      0x80, 0x15, 0xd9, 0xb7, 0x2d, 0x7d, 0x57, 0x24, 0x4e, 0xa8, 0xef, 0x9a,
      0xc0, 0xc6, 0x21, 0x89, 0x67, 0x08, 0xa5, 0x93, 0x67, 0xf9, 0xdf, 0xb9,
      0xf5, 0x4c, 0xa8, 0x4b, 0x3f, 0x1c, 0x9d, 0xb1, 0x28, 0x8b, 0x23, 0x1c,
      0x3a, 0xe0, 0xd4, 0xfe, 0x73, 0x44, 0xfd, 0x25, 0x33, 0x26, 0x47, 0x20};

  fio___p384_point_affine_s q;
  fio___p384_fe_from_bytes(q.x, qx_bytes);
  fio___p384_fe_from_bytes(q.y, qy_bytes);

  /* Test: 1*G + 0*Q = G */
  {
    fio___p384_scalar_s u1 = {1, 0, 0, 0, 0, 0};
    fio___p384_scalar_s u2 = {0, 0, 0, 0, 0, 0};

    fio___p384_point_jacobian_s R_jac;
    fio___p384_point_mul2(&R_jac, u1, u2, &q);

    fio___p384_point_affine_s R;
    fio___p384_point_to_affine(&R, &R_jac);

    int x_match = (fio___p384_fe_eq(R.x, FIO___P384_GX) == 0);
    int y_match = (fio___p384_fe_eq(R.y, FIO___P384_GY) == 0);

    fprintf(stderr,
            "1*G + 0*Q = G: %s\n",
            (x_match && y_match) ? "PASS" : "FAIL");
    if (!x_match || !y_match) {
      print_fe384("Result.x", R.x);
      print_fe384("G.x     ", FIO___P384_GX);
    }
  }

  /* Test: 0*G + 1*Q = Q */
  {
    fio___p384_scalar_s u1 = {0, 0, 0, 0, 0, 0};
    fio___p384_scalar_s u2 = {1, 0, 0, 0, 0, 0};

    fio___p384_point_jacobian_s R_jac;
    fio___p384_point_mul2(&R_jac, u1, u2, &q);

    fio___p384_point_affine_s R;
    fio___p384_point_to_affine(&R, &R_jac);

    int x_match = (fio___p384_fe_eq(R.x, q.x) == 0);
    int y_match = (fio___p384_fe_eq(R.y, q.y) == 0);

    fprintf(stderr,
            "0*G + 1*Q = Q: %s\n",
            (x_match && y_match) ? "PASS" : "FAIL");
    if (!x_match || !y_match) {
      print_fe384("Result.x", R.x);
      print_fe384("Q.x     ", q.x);
    }
  }

  /* Test: 1*G + 1*Q = G + Q */
  {
    fio___p384_scalar_s u1 = {1, 0, 0, 0, 0, 0};
    fio___p384_scalar_s u2 = {1, 0, 0, 0, 0, 0};

    /* Compute expected G + Q */
    fio___p384_point_affine_s g;
    fio___p384_fe_copy(g.x, FIO___P384_GX);
    fio___p384_fe_copy(g.y, FIO___P384_GY);

    fio___p384_point_jacobian_s expected_jac;
    fio___p384_point_to_jacobian(&expected_jac, &g);
    fio___p384_point_add_mixed(&expected_jac, &expected_jac, &q);
    fio___p384_point_affine_s expected;
    fio___p384_point_to_affine(&expected, &expected_jac);

    /* Compute via mul2 */
    fio___p384_point_jacobian_s R_jac;
    fio___p384_point_mul2(&R_jac, u1, u2, &q);
    fio___p384_point_affine_s R;
    fio___p384_point_to_affine(&R, &R_jac);

    int x_match = (fio___p384_fe_eq(R.x, expected.x) == 0);
    int y_match = (fio___p384_fe_eq(R.y, expected.y) == 0);

    fprintf(stderr,
            "1*G + 1*Q = G+Q: %s\n",
            (x_match && y_match) ? "PASS" : "FAIL");
    if (!x_match || !y_match) {
      print_fe384("Result.x  ", R.x);
      print_fe384("Expected.x", expected.x);
    }
  }

  /* Test: 2*G + 0*Q = 2G */
  {
    fio___p384_scalar_s u1 = {2, 0, 0, 0, 0, 0};
    fio___p384_scalar_s u2 = {0, 0, 0, 0, 0, 0};

    /* Compute expected 2G */
    fio___p384_point_affine_s g;
    fio___p384_fe_copy(g.x, FIO___P384_GX);
    fio___p384_fe_copy(g.y, FIO___P384_GY);

    fio___p384_point_jacobian_s expected_jac;
    fio___p384_point_to_jacobian(&expected_jac, &g);
    fio___p384_point_double(&expected_jac, &expected_jac);
    fio___p384_point_affine_s expected;
    fio___p384_point_to_affine(&expected, &expected_jac);

    /* Compute via mul2 */
    fio___p384_point_jacobian_s R_jac;
    fio___p384_point_mul2(&R_jac, u1, u2, &q);
    fio___p384_point_affine_s R;
    fio___p384_point_to_affine(&R, &R_jac);

    int x_match = (fio___p384_fe_eq(R.x, expected.x) == 0);
    int y_match = (fio___p384_fe_eq(R.y, expected.y) == 0);

    fprintf(stderr,
            "2*G + 0*Q = 2G: %s\n",
            (x_match && y_match) ? "PASS" : "FAIL");
    if (!x_match || !y_match) {
      print_fe384("Result.x  ", R.x);
      print_fe384("Expected.x", expected.x);
    }
  }

  fprintf(stderr, "\n");
}

/* *****************************************************************************
Test 8: Verify scalar arithmetic (mod n)
***************************************************************************** */
FIO_SFUNC void test_scalar_arithmetic(void) {
  FIO_LOG_INFO("=== Test 8: Scalar Arithmetic (mod n) ===");

  /* Test s * s^(-1) = 1 */
  static const uint8_t s_bytes[48] = {
      0x99, 0xef, 0x4a, 0xeb, 0x15, 0xf1, 0x78, 0xce, 0xa1, 0xfe, 0x40, 0xdb,
      0x26, 0x03, 0x13, 0x8f, 0x13, 0x0e, 0x74, 0x0a, 0x19, 0x62, 0x45, 0x26,
      0x20, 0x3b, 0x63, 0x51, 0xd0, 0xa3, 0xa9, 0x4f, 0xa3, 0x29, 0xc1, 0x45,
      0x78, 0x6e, 0x67, 0x9e, 0x7b, 0x82, 0xc7, 0x1a, 0x38, 0x62, 0x8a, 0xc8};

  fio___p384_scalar_s s, s_inv, product;
  fio___p384_scalar_from_bytes(s, s_bytes);
  fio___p384_scalar_inv(s_inv, s);
  fio___p384_scalar_mul(product, s, s_inv);

  print_fe384("s", s);
  print_fe384("s^(-1)", s_inv);
  print_fe384("s * s^(-1)", product);

  int is_one = (product[0] == 1 && product[1] == 0 && product[2] == 0 &&
                product[3] == 0 && product[4] == 0 && product[5] == 0);
  fprintf(stderr, "s * s^(-1) = 1: %s\n", is_one ? "PASS" : "FAIL");

  fprintf(stderr, "\n");
}

/* *****************************************************************************
Test 9: Verify the private key d generates the public key Q
RFC 6979 A.2.6 provides the private key:
d =
6B9D3DAD2E1B8C1C05B19875B6659F4DE23C3B667BF297BA9AA47740787137D896D5724E4C70A825F872C9EA60D2EDF5
***************************************************************************** */
FIO_SFUNC void test_private_key_generates_pubkey(void) {
  FIO_LOG_INFO(
      "=== Test 9: Verify d*G = Q (private key generates public key) ===");

  /* Private key d from RFC 6979 A.2.6 */
  static const uint8_t d_bytes[48] = {
      0x6b, 0x9d, 0x3d, 0xad, 0x2e, 0x1b, 0x8c, 0x1c, 0x05, 0xb1, 0x98, 0x75,
      0xb6, 0x65, 0x9f, 0x4d, 0xe2, 0x3c, 0x3b, 0x66, 0x7b, 0xf2, 0x97, 0xba,
      0x9a, 0xa4, 0x77, 0x40, 0x78, 0x71, 0x37, 0xd8, 0x96, 0xd5, 0x72, 0x4e,
      0x4c, 0x70, 0xa8, 0x25, 0xf8, 0x72, 0xc9, 0xea, 0x60, 0xd2, 0xed, 0xf5};

  /* Expected public key Q from RFC 6979 A.2.6 */
  static const uint8_t qx_bytes[48] = {
      0xec, 0x3a, 0x4e, 0x41, 0x5b, 0x4e, 0x19, 0xa4, 0x56, 0x86, 0x18, 0x02,
      0x9f, 0x42, 0x7f, 0xa5, 0xda, 0x9a, 0x8b, 0xc4, 0xae, 0x92, 0xe0, 0x2e,
      0x06, 0xaa, 0xe5, 0x28, 0x6b, 0x30, 0x0c, 0x64, 0xde, 0xf8, 0xf0, 0xea,
      0x90, 0x55, 0x86, 0x60, 0x64, 0xa2, 0x54, 0x51, 0x54, 0x80, 0xbc, 0x13};
  static const uint8_t qy_bytes[48] = {
      0x80, 0x15, 0xd9, 0xb7, 0x2d, 0x7d, 0x57, 0x24, 0x4e, 0xa8, 0xef, 0x9a,
      0xc0, 0xc6, 0x21, 0x89, 0x67, 0x08, 0xa5, 0x93, 0x67, 0xf9, 0xdf, 0xb9,
      0xf5, 0x4c, 0xa8, 0x4b, 0x3f, 0x1c, 0x9d, 0xb1, 0x28, 0x8b, 0x23, 0x1c,
      0x3a, 0xe0, 0xd4, 0xfe, 0x73, 0x44, 0xfd, 0x25, 0x33, 0x26, 0x47, 0x20};

  fio___p384_scalar_s d;
  fio___p384_scalar_from_bytes(d, d_bytes);

  fio___p384_fe_s expected_qx, expected_qy;
  fio___p384_fe_from_bytes(expected_qx, qx_bytes);
  fio___p384_fe_from_bytes(expected_qy, qy_bytes);

  /* Compute d*G */
  fio___p384_point_affine_s g;
  fio___p384_fe_copy(g.x, FIO___P384_GX);
  fio___p384_fe_copy(g.y, FIO___P384_GY);

  fio___p384_point_jacobian_s result_jac;
  fio___p384_point_mul(&result_jac, d, &g);

  fio___p384_point_affine_s result;
  fio___p384_point_to_affine(&result, &result_jac);

  print_fe384("d", d);
  print_fe384("d*G.x (computed)", result.x);
  print_fe384("Q.x   (expected)", expected_qx);
  print_fe384("d*G.y (computed)", result.y);
  print_fe384("Q.y   (expected)", expected_qy);

  int x_match = (fio___p384_fe_eq(result.x, expected_qx) == 0);
  int y_match = (fio___p384_fe_eq(result.y, expected_qy) == 0);
  int on_curve = point_on_curve(&result);

  fprintf(stderr, "d*G on curve: %s\n", on_curve ? "YES" : "NO");
  fprintf(stderr, "d*G = Q: %s\n", (x_match && y_match) ? "PASS" : "FAIL");

  fprintf(stderr, "\n");
}

/* *****************************************************************************
Test 10: Verify signature generation matches expected r
For ECDSA, r = (k*G).x mod n where k is the nonce
RFC 6979 A.2.6 provides k for the "sample" message
k =
94ED910D1A099DAD3254E9242AE85ABDE4BA15168EAF0CA87A555FD56D10FBCA2907E3E83BA95368623B8C4686915CF9
***************************************************************************** */
FIO_SFUNC void test_signature_generation(void) {
  FIO_LOG_INFO(
      "=== Test 10: Verify k*G.x mod n = r (signature generation) ===");

  /* Nonce k from RFC 6979 A.2.6 for message "sample" */
  static const uint8_t k_bytes[48] = {
      0x94, 0xed, 0x91, 0x0d, 0x1a, 0x09, 0x9d, 0xad, 0x32, 0x54, 0xe9, 0x24,
      0x2a, 0xe8, 0x5a, 0xbd, 0xe4, 0xba, 0x15, 0x16, 0x8e, 0xaf, 0x0c, 0xa8,
      0x7a, 0x55, 0x5f, 0xd5, 0x6d, 0x10, 0xfb, 0xca, 0x29, 0x07, 0xe3, 0xe8,
      0x3b, 0xa9, 0x53, 0x68, 0x62, 0x3b, 0x8c, 0x46, 0x86, 0x91, 0x5c, 0xf9};

  /* Expected r from RFC 6979 A.2.6 */
  static const uint8_t r_bytes[48] = {
      0x94, 0xed, 0xbb, 0x92, 0xa5, 0xec, 0xb8, 0xaa, 0xd4, 0x73, 0x6e, 0x56,
      0xc6, 0x91, 0x91, 0x6b, 0x3f, 0x88, 0x14, 0x06, 0x66, 0xce, 0x9f, 0xa7,
      0x3d, 0x64, 0xc4, 0xea, 0x95, 0xad, 0x13, 0x3c, 0x81, 0xa6, 0x48, 0x15,
      0x2e, 0x44, 0xac, 0xf9, 0x6e, 0x36, 0xdd, 0x1e, 0x80, 0xfa, 0xbe, 0x46};

  fio___p384_scalar_s k, expected_r;
  fio___p384_scalar_from_bytes(k, k_bytes);
  fio___p384_scalar_from_bytes(expected_r, r_bytes);

  /* Compute k*G */
  fio___p384_point_affine_s g;
  fio___p384_fe_copy(g.x, FIO___P384_GX);
  fio___p384_fe_copy(g.y, FIO___P384_GY);

  fio___p384_point_jacobian_s result_jac;
  fio___p384_point_mul(&result_jac, k, &g);

  fio___p384_point_affine_s result;
  fio___p384_point_to_affine(&result, &result_jac);

  /* Get (k*G).x mod n */
  uint8_t rx_bytes[48];
  fio___p384_fe_to_bytes(rx_bytes, result.x);
  fio___p384_scalar_s rx;
  fio___p384_scalar_from_bytes(rx, rx_bytes);
  while (fio___p384_scalar_gte_n(rx)) {
    uint64_t borrow = 0;
    rx[0] = fio_math_subc64(rx[0], FIO___P384_N[0], 0, &borrow);
    rx[1] = fio_math_subc64(rx[1], FIO___P384_N[1], borrow, &borrow);
    rx[2] = fio_math_subc64(rx[2], FIO___P384_N[2], borrow, &borrow);
    rx[3] = fio_math_subc64(rx[3], FIO___P384_N[3], borrow, &borrow);
    rx[4] = fio_math_subc64(rx[4], FIO___P384_N[4], borrow, &borrow);
    rx[5] = fio_math_subc64(rx[5], FIO___P384_N[5], borrow, &borrow);
  }

  print_fe384("k", k);
  print_fe384("(k*G).x", result.x);
  print_fe384("(k*G).x mod n (computed r)", rx);
  print_fe384("r (expected)", expected_r);

  int match = (rx[0] == expected_r[0] && rx[1] == expected_r[1] &&
               rx[2] == expected_r[2] && rx[3] == expected_r[3] &&
               rx[4] == expected_r[4] && rx[5] == expected_r[5]);
  fprintf(stderr, "k*G.x mod n = r: %s\n", match ? "PASS" : "FAIL");

  fprintf(stderr, "\n");
}

/* *****************************************************************************
Test 11: Verify the full ECDSA verification equation
For a valid signature: (u1*G + u2*Q).x mod n = r
where u1 = e*w, u2 = r*w, w = s^(-1), e = hash
This should equal r = (k*G).x mod n
***************************************************************************** */
FIO_SFUNC void test_ecdsa_equation(void) {
  FIO_LOG_INFO("=== Test 11: Full ECDSA verification equation ===");

  /* clang-format off */
  /* All values from RFC 6979 A.2.6 */
  
  /* Private key d */
  static const uint8_t d_bytes[48] = {
      0x6b, 0x9d, 0x3d, 0xad, 0x2e, 0x1b, 0x8c, 0x1c, 0x05, 0xb1, 0x98, 0x75,
      0xb6, 0x65, 0x9f, 0x4d, 0xe2, 0x3c, 0x3b, 0x66, 0x7b, 0xf2, 0x97, 0xba,
      0x9a, 0xa4, 0x77, 0x40, 0x78, 0x71, 0x37, 0xd8, 0x96, 0xd5, 0x72, 0x4e,
      0x4c, 0x70, 0xa8, 0x25, 0xf8, 0x72, 0xc9, 0xea, 0x60, 0xd2, 0xed, 0xf5};

  /* Nonce k */
  static const uint8_t k_bytes[48] = {
      0x94, 0xed, 0x91, 0x0d, 0x1a, 0x09, 0x9d, 0xad, 0x32, 0x54, 0xe9, 0x24,
      0x2a, 0xe8, 0x5a, 0xbd, 0xe4, 0xba, 0x15, 0x16, 0x8e, 0xaf, 0x0c, 0xa8,
      0x7a, 0x55, 0x5f, 0xd5, 0x6d, 0x10, 0xfb, 0xca, 0x29, 0x07, 0xe3, 0xe8,
      0x3b, 0xa9, 0x53, 0x68, 0x62, 0x3b, 0x8c, 0x46, 0x86, 0x91, 0x5c, 0xf9};

  /* SHA-384("sample") */
  static const uint8_t msg_hash[48] = {
    0xfd, 0xa5, 0x4e, 0x30, 0x04, 0x9d, 0x8a, 0xcd,
    0xd5, 0x1c, 0xa3, 0x35, 0x0b, 0x97, 0x8a, 0x2d,
    0xc8, 0x5d, 0x3d, 0x33, 0x03, 0xf9, 0x6d, 0x81,
    0x08, 0xd4, 0x2a, 0x47, 0x67, 0x99, 0xd6, 0x12,
    0x22, 0xc2, 0x4c, 0xb4, 0x84, 0xe3, 0x1b, 0xd8,
    0xf1, 0xec, 0xb4, 0x10, 0x49, 0x1e, 0xdc, 0x21
  };

  static const uint8_t r_bytes[48] = {
    0x94, 0xed, 0xbb, 0x92, 0xa5, 0xec, 0xb8, 0xaa,
    0xd4, 0x73, 0x6e, 0x56, 0xc6, 0x91, 0x91, 0x6b,
    0x3f, 0x88, 0x14, 0x06, 0x66, 0xce, 0x9f, 0xa7,
    0x3d, 0x64, 0xc4, 0xea, 0x95, 0xad, 0x13, 0x3c,
    0x81, 0xa6, 0x48, 0x15, 0x2e, 0x44, 0xac, 0xf9,
    0x6e, 0x36, 0xdd, 0x1e, 0x80, 0xfa, 0xbe, 0x46
  };
  static const uint8_t s_bytes[48] = {
    0x99, 0xef, 0x4a, 0xeb, 0x15, 0xf1, 0x78, 0xce,
    0xa1, 0xfe, 0x40, 0xdb, 0x26, 0x03, 0x13, 0x8f,
    0x13, 0x0e, 0x74, 0x0a, 0x19, 0x62, 0x45, 0x26,
    0x20, 0x3b, 0x63, 0x51, 0xd0, 0xa3, 0xa9, 0x4f,
    0xa3, 0x29, 0xc1, 0x45, 0x78, 0x6e, 0x67, 0x9e,
    0x7b, 0x82, 0xc7, 0x1a, 0x38, 0x62, 0x8a, 0xc8
  };
  /* clang-format on */

  fio___p384_scalar_s d, k, e, r, s;
  fio___p384_scalar_from_bytes(d, d_bytes);
  fio___p384_scalar_from_bytes(k, k_bytes);
  fio___p384_scalar_from_bytes(e, msg_hash);
  fio___p384_scalar_from_bytes(r, r_bytes);
  fio___p384_scalar_from_bytes(s, s_bytes);

  /* Verify the signature equation: s = k^(-1) * (e + r*d) mod n */
  FIO_LOG_INFO("  Verifying signature equation: s = k^(-1) * (e + r*d) mod n");
  {
    fio___p384_scalar_s k_inv, rd;

    /* k^(-1) */
    fio___p384_scalar_inv(k_inv, k);

    /* r*d */
    fio___p384_scalar_mul(rd, r, d);

    /* e + r*d - need to implement scalar addition */
    /* For now, let's just verify the verification side */
  }

  /* Verify: u1*G + u2*Q should equal k*G */
  FIO_LOG_INFO("  Verifying: u1*G + u2*Q = k*G");

  /* Compute Q = d*G */
  fio___p384_point_affine_s g;
  fio___p384_fe_copy(g.x, FIO___P384_GX);
  fio___p384_fe_copy(g.y, FIO___P384_GY);

  fio___p384_point_jacobian_s q_jac;
  fio___p384_point_mul(&q_jac, d, &g);
  fio___p384_point_affine_s q;
  fio___p384_point_to_affine(&q, &q_jac);

  /* Compute k*G (the expected result) */
  fio___p384_point_jacobian_s kg_jac;
  fio___p384_point_mul(&kg_jac, k, &g);
  fio___p384_point_affine_s kg;
  fio___p384_point_to_affine(&kg, &kg_jac);

  print_fe384("k*G.x (expected R)", kg.x);

  /* Compute w = s^(-1) */
  fio___p384_scalar_s w;
  fio___p384_scalar_inv(w, s);

  /* Compute u1 = e * w mod n */
  fio___p384_scalar_s u1;
  fio___p384_scalar_mul(u1, e, w);

  /* Compute u2 = r * w mod n */
  fio___p384_scalar_s u2;
  fio___p384_scalar_mul(u2, r, w);

  print_fe384("u1 = e*w", u1);
  print_fe384("u2 = r*w", u2);

  /* Compute u1*G + u2*Q */
  fio___p384_point_jacobian_s R_jac;
  fio___p384_point_mul2(&R_jac, u1, u2, &q);
  fio___p384_point_affine_s R;
  fio___p384_point_to_affine(&R, &R_jac);

  print_fe384("(u1*G + u2*Q).x (computed R)", R.x);

  int x_match = (fio___p384_fe_eq(R.x, kg.x) == 0);
  int y_match = (fio___p384_fe_eq(R.y, kg.y) == 0);

  fprintf(stderr,
          "u1*G + u2*Q = k*G: %s\n",
          (x_match && y_match) ? "PASS" : "FAIL");

  /* The key insight: for ECDSA verification to work, we need:
   * u1*G + u2*Q = k*G
   *
   * Since Q = d*G, this becomes:
   * u1*G + u2*d*G = k*G
   * (u1 + u2*d)*G = k*G
   * u1 + u2*d = k (mod n)
   *
   * Substituting u1 = e*w and u2 = r*w where w = s^(-1):
   * e*w + r*w*d = k
   * w*(e + r*d) = k
   * (e + r*d)/s = k
   * e + r*d = k*s
   * s = (e + r*d)/k = k^(-1)*(e + r*d)
   *
   * This is exactly the signature equation!
   */

  fprintf(stderr, "\n");
}

/* *****************************************************************************
Test 12: Verify u1 + u2*d = k (mod n)
This is the core ECDSA identity
***************************************************************************** */
FIO_SFUNC void test_ecdsa_identity(void) {
  FIO_LOG_INFO("=== Test 12: Verify u1 + u2*d = k (mod n) ===");

  /* clang-format off */
  static const uint8_t d_bytes[48] = {
      0x6b, 0x9d, 0x3d, 0xad, 0x2e, 0x1b, 0x8c, 0x1c, 0x05, 0xb1, 0x98, 0x75,
      0xb6, 0x65, 0x9f, 0x4d, 0xe2, 0x3c, 0x3b, 0x66, 0x7b, 0xf2, 0x97, 0xba,
      0x9a, 0xa4, 0x77, 0x40, 0x78, 0x71, 0x37, 0xd8, 0x96, 0xd5, 0x72, 0x4e,
      0x4c, 0x70, 0xa8, 0x25, 0xf8, 0x72, 0xc9, 0xea, 0x60, 0xd2, 0xed, 0xf5};

  static const uint8_t k_bytes[48] = {
      0x94, 0xed, 0x91, 0x0d, 0x1a, 0x09, 0x9d, 0xad, 0x32, 0x54, 0xe9, 0x24,
      0x2a, 0xe8, 0x5a, 0xbd, 0xe4, 0xba, 0x15, 0x16, 0x8e, 0xaf, 0x0c, 0xa8,
      0x7a, 0x55, 0x5f, 0xd5, 0x6d, 0x10, 0xfb, 0xca, 0x29, 0x07, 0xe3, 0xe8,
      0x3b, 0xa9, 0x53, 0x68, 0x62, 0x3b, 0x8c, 0x46, 0x86, 0x91, 0x5c, 0xf9};

  static const uint8_t msg_hash[48] = {
    0xfd, 0xa5, 0x4e, 0x30, 0x04, 0x9d, 0x8a, 0xcd,
    0xd5, 0x1c, 0xa3, 0x35, 0x0b, 0x97, 0x8a, 0x2d,
    0xc8, 0x5d, 0x3d, 0x33, 0x03, 0xf9, 0x6d, 0x81,
    0x08, 0xd4, 0x2a, 0x47, 0x67, 0x99, 0xd6, 0x12,
    0x22, 0xc2, 0x4c, 0xb4, 0x84, 0xe3, 0x1b, 0xd8,
    0xf1, 0xec, 0xb4, 0x10, 0x49, 0x1e, 0xdc, 0x21
  };

  static const uint8_t r_bytes[48] = {
    0x94, 0xed, 0xbb, 0x92, 0xa5, 0xec, 0xb8, 0xaa,
    0xd4, 0x73, 0x6e, 0x56, 0xc6, 0x91, 0x91, 0x6b,
    0x3f, 0x88, 0x14, 0x06, 0x66, 0xce, 0x9f, 0xa7,
    0x3d, 0x64, 0xc4, 0xea, 0x95, 0xad, 0x13, 0x3c,
    0x81, 0xa6, 0x48, 0x15, 0x2e, 0x44, 0xac, 0xf9,
    0x6e, 0x36, 0xdd, 0x1e, 0x80, 0xfa, 0xbe, 0x46
  };
  static const uint8_t s_bytes[48] = {
    0x99, 0xef, 0x4a, 0xeb, 0x15, 0xf1, 0x78, 0xce,
    0xa1, 0xfe, 0x40, 0xdb, 0x26, 0x03, 0x13, 0x8f,
    0x13, 0x0e, 0x74, 0x0a, 0x19, 0x62, 0x45, 0x26,
    0x20, 0x3b, 0x63, 0x51, 0xd0, 0xa3, 0xa9, 0x4f,
    0xa3, 0x29, 0xc1, 0x45, 0x78, 0x6e, 0x67, 0x9e,
    0x7b, 0x82, 0xc7, 0x1a, 0x38, 0x62, 0x8a, 0xc8
  };
  /* clang-format on */

  fio___p384_scalar_s d, k, e, r, s;
  fio___p384_scalar_from_bytes(d, d_bytes);
  fio___p384_scalar_from_bytes(k, k_bytes);
  fio___p384_scalar_from_bytes(e, msg_hash);
  fio___p384_scalar_from_bytes(r, r_bytes);
  fio___p384_scalar_from_bytes(s, s_bytes);

  /* Compute w = s^(-1) */
  fio___p384_scalar_s w;
  fio___p384_scalar_inv(w, s);

  /* Compute u1 = e * w mod n */
  fio___p384_scalar_s u1;
  fio___p384_scalar_mul(u1, e, w);

  /* Compute u2 = r * w mod n */
  fio___p384_scalar_s u2;
  fio___p384_scalar_mul(u2, r, w);

  /* Compute u2 * d */
  fio___p384_scalar_s u2d;
  fio___p384_scalar_mul(u2d, u2, d);

  print_fe384("u1", u1);
  print_fe384("u2", u2);
  print_fe384("d", d);
  print_fe384("u2*d", u2d);
  print_fe384("k (expected)", k);

  /* Compute u1 + u2*d mod n using schoolbook addition */
  /* We need to add u1 + u2d and reduce mod n */
  uint64_t sum[7] = {0}; /* 7 limbs to handle carry */
  uint64_t carry = 0;
  sum[0] = fio_math_addc64(u1[0], u2d[0], 0, &carry);
  sum[1] = fio_math_addc64(u1[1], u2d[1], carry, &carry);
  sum[2] = fio_math_addc64(u1[2], u2d[2], carry, &carry);
  sum[3] = fio_math_addc64(u1[3], u2d[3], carry, &carry);
  sum[4] = fio_math_addc64(u1[4], u2d[4], carry, &carry);
  sum[5] = fio_math_addc64(u1[5], u2d[5], carry, &carry);
  sum[6] = carry;

  /* Reduce mod n if needed */
  while (sum[6] > 0 || (sum[5] > FIO___P384_N[5]) ||
         (sum[5] == FIO___P384_N[5] && sum[4] > FIO___P384_N[4]) ||
         (sum[5] == FIO___P384_N[5] && sum[4] == FIO___P384_N[4] &&
          sum[3] > FIO___P384_N[3]) ||
         (sum[5] == FIO___P384_N[5] && sum[4] == FIO___P384_N[4] &&
          sum[3] == FIO___P384_N[3] && sum[2] > FIO___P384_N[2]) ||
         (sum[5] == FIO___P384_N[5] && sum[4] == FIO___P384_N[4] &&
          sum[3] == FIO___P384_N[3] && sum[2] == FIO___P384_N[2] &&
          sum[1] > FIO___P384_N[1]) ||
         (sum[5] == FIO___P384_N[5] && sum[4] == FIO___P384_N[4] &&
          sum[3] == FIO___P384_N[3] && sum[2] == FIO___P384_N[2] &&
          sum[1] == FIO___P384_N[1] && sum[0] >= FIO___P384_N[0])) {
    uint64_t borrow = 0;
    sum[0] = fio_math_subc64(sum[0], FIO___P384_N[0], 0, &borrow);
    sum[1] = fio_math_subc64(sum[1], FIO___P384_N[1], borrow, &borrow);
    sum[2] = fio_math_subc64(sum[2], FIO___P384_N[2], borrow, &borrow);
    sum[3] = fio_math_subc64(sum[3], FIO___P384_N[3], borrow, &borrow);
    sum[4] = fio_math_subc64(sum[4], FIO___P384_N[4], borrow, &borrow);
    sum[5] = fio_math_subc64(sum[5], FIO___P384_N[5], borrow, &borrow);
    sum[6] -= borrow;
  }

  fio___p384_scalar_s result;
  result[0] = sum[0];
  result[1] = sum[1];
  result[2] = sum[2];
  result[3] = sum[3];
  result[4] = sum[4];
  result[5] = sum[5];

  print_fe384("u1 + u2*d (computed)", result);

  int match = (result[0] == k[0] && result[1] == k[1] && result[2] == k[2] &&
               result[3] == k[3] && result[4] == k[4] && result[5] == k[5]);
  fprintf(stderr, "u1 + u2*d = k: %s\n", match ? "PASS" : "FAIL");

  if (!match) {
    FIO_LOG_ERROR("  The ECDSA identity u1 + u2*d = k does NOT hold!");
    FIO_LOG_ERROR(
        "  This means either u1, u2, or the scalar arithmetic is wrong.");
  }

  fprintf(stderr, "\n");
}

/* *****************************************************************************
Test 13: Verify scalar multiplication in detail
Check if e*w and r*w are computed correctly
***************************************************************************** */
FIO_SFUNC void test_scalar_mul_detail(void) {
  FIO_LOG_INFO("=== Test 13: Detailed scalar multiplication verification ===");

  /* clang-format off */
  static const uint8_t msg_hash[48] = {
    0xfd, 0xa5, 0x4e, 0x30, 0x04, 0x9d, 0x8a, 0xcd,
    0xd5, 0x1c, 0xa3, 0x35, 0x0b, 0x97, 0x8a, 0x2d,
    0xc8, 0x5d, 0x3d, 0x33, 0x03, 0xf9, 0x6d, 0x81,
    0x08, 0xd4, 0x2a, 0x47, 0x67, 0x99, 0xd6, 0x12,
    0x22, 0xc2, 0x4c, 0xb4, 0x84, 0xe3, 0x1b, 0xd8,
    0xf1, 0xec, 0xb4, 0x10, 0x49, 0x1e, 0xdc, 0x21
  };

  static const uint8_t r_bytes[48] = {
    0x94, 0xed, 0xbb, 0x92, 0xa5, 0xec, 0xb8, 0xaa,
    0xd4, 0x73, 0x6e, 0x56, 0xc6, 0x91, 0x91, 0x6b,
    0x3f, 0x88, 0x14, 0x06, 0x66, 0xce, 0x9f, 0xa7,
    0x3d, 0x64, 0xc4, 0xea, 0x95, 0xad, 0x13, 0x3c,
    0x81, 0xa6, 0x48, 0x15, 0x2e, 0x44, 0xac, 0xf9,
    0x6e, 0x36, 0xdd, 0x1e, 0x80, 0xfa, 0xbe, 0x46
  };
  static const uint8_t s_bytes[48] = {
    0x99, 0xef, 0x4a, 0xeb, 0x15, 0xf1, 0x78, 0xce,
    0xa1, 0xfe, 0x40, 0xdb, 0x26, 0x03, 0x13, 0x8f,
    0x13, 0x0e, 0x74, 0x0a, 0x19, 0x62, 0x45, 0x26,
    0x20, 0x3b, 0x63, 0x51, 0xd0, 0xa3, 0xa9, 0x4f,
    0xa3, 0x29, 0xc1, 0x45, 0x78, 0x6e, 0x67, 0x9e,
    0x7b, 0x82, 0xc7, 0x1a, 0x38, 0x62, 0x8a, 0xc8
  };
  static const uint8_t k_bytes[48] = {
      0x94, 0xed, 0x91, 0x0d, 0x1a, 0x09, 0x9d, 0xad, 0x32, 0x54, 0xe9, 0x24,
      0x2a, 0xe8, 0x5a, 0xbd, 0xe4, 0xba, 0x15, 0x16, 0x8e, 0xaf, 0x0c, 0xa8,
      0x7a, 0x55, 0x5f, 0xd5, 0x6d, 0x10, 0xfb, 0xca, 0x29, 0x07, 0xe3, 0xe8,
      0x3b, 0xa9, 0x53, 0x68, 0x62, 0x3b, 0x8c, 0x46, 0x86, 0x91, 0x5c, 0xf9};
  /* clang-format on */

  fio___p384_scalar_s e, r, s, k;
  fio___p384_scalar_from_bytes(e, msg_hash);
  fio___p384_scalar_from_bytes(r, r_bytes);
  fio___p384_scalar_from_bytes(s, s_bytes);
  fio___p384_scalar_from_bytes(k, k_bytes);

  print_fe384("e (hash, raw)", e);

  /* Check if e >= n and reduce if needed */
  fprintf(stderr, "e >= n: %s\n", fio___p384_scalar_gte_n(e) ? "YES" : "NO");
  if (fio___p384_scalar_gte_n(e)) {
    uint64_t borrow = 0;
    e[0] = fio_math_subc64(e[0], FIO___P384_N[0], 0, &borrow);
    e[1] = fio_math_subc64(e[1], FIO___P384_N[1], borrow, &borrow);
    e[2] = fio_math_subc64(e[2], FIO___P384_N[2], borrow, &borrow);
    e[3] = fio_math_subc64(e[3], FIO___P384_N[3], borrow, &borrow);
    e[4] = fio_math_subc64(e[4], FIO___P384_N[4], borrow, &borrow);
    e[5] = fio_math_subc64(e[5], FIO___P384_N[5], borrow, &borrow);
    fprintf(stderr, "e reduced mod n\n");
  }

  print_fe384("e (hash, reduced)", e);
  print_fe384("r", r);
  print_fe384("s", s);
  print_fe384("k", k);

  /* Verify: s * k = e + r*d (mod n) */
  /* This means: k = s^(-1) * (e + r*d) */
  /* And: s = k^(-1) * (e + r*d) */

  /* Let's verify: k * s should equal e + r*d */
  fio___p384_scalar_s ks;
  fio___p384_scalar_mul(ks, k, s);
  print_fe384("k * s", ks);

  /* Now let's check if s^(-1) * k * s = k */
  fio___p384_scalar_s s_inv;
  fio___p384_scalar_inv(s_inv, s);

  fio___p384_scalar_s s_inv_ks;
  fio___p384_scalar_mul(s_inv_ks, s_inv, ks);
  print_fe384("s^(-1) * (k*s)", s_inv_ks);

  int match =
      (s_inv_ks[0] == k[0] && s_inv_ks[1] == k[1] && s_inv_ks[2] == k[2] &&
       s_inv_ks[3] == k[3] && s_inv_ks[4] == k[4] && s_inv_ks[5] == k[5]);
  fprintf(stderr, "s^(-1) * (k*s) = k: %s\n", match ? "PASS" : "FAIL");

  /* Now let's verify the ECDSA equation differently:
   * u1 = e * s^(-1)
   * u2 = r * s^(-1)
   *
   * For verification to work:
   * u1 + u2*d = k
   * e*s^(-1) + r*s^(-1)*d = k
   * s^(-1) * (e + r*d) = k
   * e + r*d = k*s
   *
   * So let's verify: e + r*d = k*s
   */

  /* First compute r*d */
  static const uint8_t d_bytes[48] = {
      0x6b, 0x9d, 0x3d, 0xad, 0x2e, 0x1b, 0x8c, 0x1c, 0x05, 0xb1, 0x98, 0x75,
      0xb6, 0x65, 0x9f, 0x4d, 0xe2, 0x3c, 0x3b, 0x66, 0x7b, 0xf2, 0x97, 0xba,
      0x9a, 0xa4, 0x77, 0x40, 0x78, 0x71, 0x37, 0xd8, 0x96, 0xd5, 0x72, 0x4e,
      0x4c, 0x70, 0xa8, 0x25, 0xf8, 0x72, 0xc9, 0xea, 0x60, 0xd2, 0xed, 0xf5};

  fio___p384_scalar_s d;
  fio___p384_scalar_from_bytes(d, d_bytes);

  fio___p384_scalar_s rd;
  fio___p384_scalar_mul(rd, r, d);
  print_fe384("r*d", rd);

  /* Compute e + r*d using addition */
  uint64_t sum[7] = {0};
  uint64_t carry = 0;
  sum[0] = fio_math_addc64(e[0], rd[0], 0, &carry);
  sum[1] = fio_math_addc64(e[1], rd[1], carry, &carry);
  sum[2] = fio_math_addc64(e[2], rd[2], carry, &carry);
  sum[3] = fio_math_addc64(e[3], rd[3], carry, &carry);
  sum[4] = fio_math_addc64(e[4], rd[4], carry, &carry);
  sum[5] = fio_math_addc64(e[5], rd[5], carry, &carry);
  sum[6] = carry;

  /* Reduce mod n */
  while (sum[6] > 0 || (sum[5] > FIO___P384_N[5]) ||
         (sum[5] == FIO___P384_N[5] && sum[4] > FIO___P384_N[4]) ||
         (sum[5] == FIO___P384_N[5] && sum[4] == FIO___P384_N[4] &&
          sum[3] > FIO___P384_N[3]) ||
         (sum[5] == FIO___P384_N[5] && sum[4] == FIO___P384_N[4] &&
          sum[3] == FIO___P384_N[3] && sum[2] > FIO___P384_N[2]) ||
         (sum[5] == FIO___P384_N[5] && sum[4] == FIO___P384_N[4] &&
          sum[3] == FIO___P384_N[3] && sum[2] == FIO___P384_N[2] &&
          sum[1] > FIO___P384_N[1]) ||
         (sum[5] == FIO___P384_N[5] && sum[4] == FIO___P384_N[4] &&
          sum[3] == FIO___P384_N[3] && sum[2] == FIO___P384_N[2] &&
          sum[1] == FIO___P384_N[1] && sum[0] >= FIO___P384_N[0])) {
    uint64_t borrow = 0;
    sum[0] = fio_math_subc64(sum[0], FIO___P384_N[0], 0, &borrow);
    sum[1] = fio_math_subc64(sum[1], FIO___P384_N[1], borrow, &borrow);
    sum[2] = fio_math_subc64(sum[2], FIO___P384_N[2], borrow, &borrow);
    sum[3] = fio_math_subc64(sum[3], FIO___P384_N[3], borrow, &borrow);
    sum[4] = fio_math_subc64(sum[4], FIO___P384_N[4], borrow, &borrow);
    sum[5] = fio_math_subc64(sum[5], FIO___P384_N[5], borrow, &borrow);
    sum[6] -= borrow;
  }

  fio___p384_scalar_s e_plus_rd;
  e_plus_rd[0] = sum[0];
  e_plus_rd[1] = sum[1];
  e_plus_rd[2] = sum[2];
  e_plus_rd[3] = sum[3];
  e_plus_rd[4] = sum[4];
  e_plus_rd[5] = sum[5];

  print_fe384("e + r*d", e_plus_rd);
  print_fe384("k*s (should equal e+r*d)", ks);

  match = (e_plus_rd[0] == ks[0] && e_plus_rd[1] == ks[1] &&
           e_plus_rd[2] == ks[2] && e_plus_rd[3] == ks[3] &&
           e_plus_rd[4] == ks[4] && e_plus_rd[5] == ks[5]);
  fprintf(stderr, "e + r*d = k*s: %s\n", match ? "PASS" : "FAIL");

  if (match) {
    FIO_LOG_INFO("  The signature equation is correct!");
    FIO_LOG_INFO("  This means the bug is in how u1 and u2 are computed.");

    /* Let's verify u1 and u2 computation step by step */
    fio___p384_scalar_s u1, u2;
    fio___p384_scalar_mul(u1, e, s_inv);
    fio___p384_scalar_mul(u2, r, s_inv);

    print_fe384("u1 = e * s^(-1)", u1);
    print_fe384("u2 = r * s^(-1)", u2);

    /* Verify: u1*s = e */
    fio___p384_scalar_s u1s;
    fio___p384_scalar_mul(u1s, u1, s);
    print_fe384("u1 * s (should = e)", u1s);

    match = (u1s[0] == e[0] && u1s[1] == e[1] && u1s[2] == e[2] &&
             u1s[3] == e[3] && u1s[4] == e[4] && u1s[5] == e[5]);
    fprintf(stderr, "u1 * s = e: %s\n", match ? "PASS" : "FAIL");

    /* Verify: u2*s = r */
    fio___p384_scalar_s u2s;
    fio___p384_scalar_mul(u2s, u2, s);
    print_fe384("u2 * s (should = r)", u2s);

    match = (u2s[0] == r[0] && u2s[1] == r[1] && u2s[2] == r[2] &&
             u2s[3] == r[3] && u2s[4] == r[4] && u2s[5] == r[5]);
    fprintf(stderr, "u2 * s = r: %s\n", match ? "PASS" : "FAIL");
  }

  fprintf(stderr, "\n");
}

/* *****************************************************************************
Test 14: Verify with CORRECT SHA-384("sample") hash
The hash in the original test file was WRONG!
Correct SHA-384("sample") =
9a9083505bc92276aec4be312696ef7bf3bf603f4bbd381196a029f340585312313bca4a9b5b890efee42c77b1ee25fe
***************************************************************************** */
FIO_SFUNC void test_correct_hash(void) {
  FIO_LOG_INFO("=== Test 14: Verify with CORRECT SHA-384('sample') hash ===");

  /* clang-format off */
  
  /* CORRECT SHA-384("sample") - computed with openssl/ruby */
  static const uint8_t correct_hash[48] = {
    0x9a, 0x90, 0x83, 0x50, 0x5b, 0xc9, 0x22, 0x76,
    0xae, 0xc4, 0xbe, 0x31, 0x26, 0x96, 0xef, 0x7b,
    0xf3, 0xbf, 0x60, 0x3f, 0x4b, 0xbd, 0x38, 0x11,
    0x96, 0xa0, 0x29, 0xf3, 0x40, 0x58, 0x53, 0x12,
    0x31, 0x3b, 0xca, 0x4a, 0x9b, 0x5b, 0x89, 0x0e,
    0xfe, 0xe4, 0x2c, 0x77, 0xb1, 0xee, 0x25, 0xfe
  };

  /* WRONG hash that was in the test file */
  static const uint8_t wrong_hash[48] = {
    0xfd, 0xa5, 0x4e, 0x30, 0x04, 0x9d, 0x8a, 0xcd,
    0xd5, 0x1c, 0xa3, 0x35, 0x0b, 0x97, 0x8a, 0x2d,
    0xc8, 0x5d, 0x3d, 0x33, 0x03, 0xf9, 0x6d, 0x81,
    0x08, 0xd4, 0x2a, 0x47, 0x67, 0x99, 0xd6, 0x12,
    0x22, 0xc2, 0x4c, 0xb4, 0x84, 0xe3, 0x1b, 0xd8,
    0xf1, 0xec, 0xb4, 0x10, 0x49, 0x1e, 0xdc, 0x21
  };

  static const uint8_t qx_bytes[48] = {
    0xec, 0x3a, 0x4e, 0x41, 0x5b, 0x4e, 0x19, 0xa4,
    0x56, 0x86, 0x18, 0x02, 0x9f, 0x42, 0x7f, 0xa5,
    0xda, 0x9a, 0x8b, 0xc4, 0xae, 0x92, 0xe0, 0x2e,
    0x06, 0xaa, 0xe5, 0x28, 0x6b, 0x30, 0x0c, 0x64,
    0xde, 0xf8, 0xf0, 0xea, 0x90, 0x55, 0x86, 0x60,
    0x64, 0xa2, 0x54, 0x51, 0x54, 0x80, 0xbc, 0x13
  };
  static const uint8_t qy_bytes[48] = {
    0x80, 0x15, 0xd9, 0xb7, 0x2d, 0x7d, 0x57, 0x24,
    0x4e, 0xa8, 0xef, 0x9a, 0xc0, 0xc6, 0x21, 0x89,
    0x67, 0x08, 0xa5, 0x93, 0x67, 0xf9, 0xdf, 0xb9,
    0xf5, 0x4c, 0xa8, 0x4b, 0x3f, 0x1c, 0x9d, 0xb1,
    0x28, 0x8b, 0x23, 0x1c, 0x3a, 0xe0, 0xd4, 0xfe,
    0x73, 0x44, 0xfd, 0x25, 0x33, 0x26, 0x47, 0x20
  };

  static const uint8_t r_bytes[48] = {
    0x94, 0xed, 0xbb, 0x92, 0xa5, 0xec, 0xb8, 0xaa,
    0xd4, 0x73, 0x6e, 0x56, 0xc6, 0x91, 0x91, 0x6b,
    0x3f, 0x88, 0x14, 0x06, 0x66, 0xce, 0x9f, 0xa7,
    0x3d, 0x64, 0xc4, 0xea, 0x95, 0xad, 0x13, 0x3c,
    0x81, 0xa6, 0x48, 0x15, 0x2e, 0x44, 0xac, 0xf9,
    0x6e, 0x36, 0xdd, 0x1e, 0x80, 0xfa, 0xbe, 0x46
  };
  static const uint8_t s_bytes[48] = {
    0x99, 0xef, 0x4a, 0xeb, 0x15, 0xf1, 0x78, 0xce,
    0xa1, 0xfe, 0x40, 0xdb, 0x26, 0x03, 0x13, 0x8f,
    0x13, 0x0e, 0x74, 0x0a, 0x19, 0x62, 0x45, 0x26,
    0x20, 0x3b, 0x63, 0x51, 0xd0, 0xa3, 0xa9, 0x4f,
    0xa3, 0x29, 0xc1, 0x45, 0x78, 0x6e, 0x67, 0x9e,
    0x7b, 0x82, 0xc7, 0x1a, 0x38, 0x62, 0x8a, 0xc8
  };
  /* clang-format on */

  print_bytes("Wrong hash (in test file)", wrong_hash, 48);
  print_bytes("Correct SHA-384('sample')", correct_hash, 48);

  /* Test with CORRECT hash */
  FIO_LOG_INFO("  Testing verification with CORRECT hash:");
  int result = fio_ecdsa_p384_verify_raw(r_bytes,
                                         s_bytes,
                                         correct_hash,
                                         qx_bytes,
                                         qy_bytes);
  fprintf(stderr,
          "Verification with correct hash: %s\n",
          result == 0 ? "PASS" : "FAIL");

  /* Test with WRONG hash (should fail) */
  FIO_LOG_INFO("  Testing verification with WRONG hash (should fail):");
  result = fio_ecdsa_p384_verify_raw(r_bytes,
                                     s_bytes,
                                     wrong_hash,
                                     qx_bytes,
                                     qy_bytes);
  fprintf(stderr,
          "Verification with wrong hash: %s (expected FAIL)\n",
          result == 0 ? "PASS" : "FAIL");

  fprintf(stderr, "\n");
}

/* *****************************************************************************
Main
***************************************************************************** */
int main(int argc, char const *argv[]) {
  (void)argc;
  (void)argv;

  FIO_LOG_INFO("P-384 Debug Tests - Isolating individual operations\n");

  test_point_doubling();
  test_scalar_mul_small();
  test_point_addition();
  test_scalar_mul_arbitrary();
  test_double_scalar_mul();
  test_gpq_precomputation();
  test_mul2_simple_cases();
  test_scalar_arithmetic();
  test_private_key_generates_pubkey();
  test_signature_generation();
  test_ecdsa_equation();
  test_ecdsa_identity();
  test_scalar_mul_detail();
  test_correct_hash();

  FIO_LOG_INFO("Debug tests complete.");
  return 0;
}
