/* *****************************************************************************
ECDSA P-384 OpenSSL Comparison Tests
***************************************************************************** */
#include "test-helpers.h"

#define FIO_P384
#define FIO_SHA2
#define FIO_RAND
#define FIO_LOG
#include FIO_INCLUDE_FILE

#ifdef HAVE_OPENSSL
#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/obj_mac.h>
#include <openssl/evp.h>

/* *****************************************************************************
Helper to print field elements for debugging
***************************************************************************** */
FIO_SFUNC void print_fe384(const char *name, const uint64_t fe[6]) {
  fprintf(stderr, "%s: ", name);
  for (int i = 5; i >= 0; --i) {
    fprintf(stderr, "%016llx", (unsigned long long)fe[i]);
  }
  fprintf(stderr, "\n");
}

FIO_SFUNC void print_bn(const char *name, const BIGNUM *bn) {
  char *hex = BN_bn2hex(bn);
  fprintf(stderr, "%s: %s\n", name, hex);
  OPENSSL_free(hex);
}

/* *****************************************************************************
Test: Compare scalar multiplication with OpenSSL
***************************************************************************** */
FIO_SFUNC void test_scalar_mul_openssl(void) {
  fprintf(stderr, "\n=== Testing scalar multiplication against OpenSSL ===\n");
  
  EC_GROUP *group = EC_GROUP_new_by_curve_name(NID_secp384r1);
  FIO_ASSERT(group, "Failed to create EC_GROUP");
  
  BN_CTX *ctx = BN_CTX_new();
  FIO_ASSERT(ctx, "Failed to create BN_CTX");
  
  /* Get generator point G */
  const EC_POINT *G = EC_GROUP_get0_generator(group);
  FIO_ASSERT(G, "Failed to get generator");
  
  /* Test: 2 * G */
  fprintf(stderr, "\n--- Test: 2 * G ---\n");
  
  BIGNUM *two = BN_new();
  BN_set_word(two, 2);
  
  EC_POINT *result_ossl = EC_POINT_new(group);
  EC_POINT_mul(group, result_ossl, two, NULL, NULL, ctx);
  
  BIGNUM *x_ossl = BN_new();
  BIGNUM *y_ossl = BN_new();
  EC_POINT_get_affine_coordinates(group, result_ossl, x_ossl, y_ossl, ctx);
  
  print_bn("OpenSSL 2G.x", x_ossl);
  print_bn("OpenSSL 2G.y", y_ossl);
  
  /* Our implementation */
  fio___p384_scalar_s k = {2, 0, 0, 0, 0, 0};
  fio___p384_point_affine_s g;
  fio___p384_fe_copy(g.x, FIO___P384_GX);
  fio___p384_fe_copy(g.y, FIO___P384_GY);
  
  fio___p384_point_jacobian_s result_jac;
  fio___p384_point_mul(&result_jac, k, &g);
  
  fio___p384_point_affine_s result_aff;
  fio___p384_point_to_affine(&result_aff, &result_jac);
  
  print_fe384("FIO 2G.x", result_aff.x);
  print_fe384("FIO 2G.y", result_aff.y);
  
  /* Compare */
  uint8_t x_bytes[48], y_bytes[48];
  fio___p384_fe_to_bytes(x_bytes, result_aff.x);
  fio___p384_fe_to_bytes(y_bytes, result_aff.y);
  
  uint8_t x_ossl_bytes[48], y_ossl_bytes[48];
  BN_bn2binpad(x_ossl, x_ossl_bytes, 48);
  BN_bn2binpad(y_ossl, y_ossl_bytes, 48);
  
  FIO_ASSERT(FIO_MEMCMP(x_bytes, x_ossl_bytes, 48) == 0, "2G.x mismatch!");
  FIO_ASSERT(FIO_MEMCMP(y_bytes, y_ossl_bytes, 48) == 0, "2G.y mismatch!");
  fprintf(stderr, "2*G matches OpenSSL!\n");
  
  BN_free(two);
  BN_free(x_ossl);
  BN_free(y_ossl);
  EC_POINT_free(result_ossl);
  
  /* Test: u1*G + u2*Q from the ECDSA test vector */
  fprintf(stderr, "\n--- Test: u1*G + u2*Q (ECDSA verification) ---\n");
  
  /* u1 and u2 from the test output */
  static const uint8_t u1_bytes[48] = {
    0x0a, 0xda, 0x43, 0x09, 0x17, 0x87, 0x08, 0x83,
    0x21, 0x88, 0x41, 0xa8, 0xbd, 0xe8, 0x3a, 0x34,
    0xa9, 0x98, 0x58, 0x9e, 0x5c, 0x82, 0x9d, 0x63,
    0xc4, 0x19, 0xb3, 0x86, 0x6c, 0x87, 0x1b, 0xb6,
    0x58, 0xf0, 0x35, 0x7f, 0xe2, 0xfe, 0xbd, 0xbe,
    0xd3, 0xba, 0xb9, 0x9f, 0xac, 0xe0, 0xb0, 0x45
  };
  
  static const uint8_t u2_bytes[48] = {
    0x9d, 0x6e, 0x63, 0xf7, 0x8a, 0x5f, 0xae, 0x28,
    0xd2, 0x51, 0xcf, 0x1f, 0xa5, 0x6e, 0xa5, 0x55,
    0x99, 0x36, 0xd0, 0x30, 0x7c, 0xa0, 0xca, 0x9a,
    0x22, 0xd9, 0x0f, 0x16, 0x95, 0x46, 0xba, 0xa7,
    0x23, 0xd2, 0x15, 0x87, 0x1b, 0x0d, 0xf2, 0xb5,
    0x8f, 0x54, 0x88, 0x87, 0x74, 0x8a, 0xfa, 0xcf
  };
  
  /* Public key Q */
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
  
  /* OpenSSL computation */
  BIGNUM *u1_bn = BN_bin2bn(u1_bytes, 48, NULL);
  BIGNUM *u2_bn = BN_bin2bn(u2_bytes, 48, NULL);
  BIGNUM *qx_bn = BN_bin2bn(qx_bytes, 48, NULL);
  BIGNUM *qy_bn = BN_bin2bn(qy_bytes, 48, NULL);
  
  EC_POINT *Q = EC_POINT_new(group);
  EC_POINT_set_affine_coordinates(group, Q, qx_bn, qy_bn, ctx);
  
  /* Compute u1*G + u2*Q */
  EC_POINT *R_ossl = EC_POINT_new(group);
  EC_POINT_mul(group, R_ossl, u1_bn, Q, u2_bn, ctx);
  
  BIGNUM *rx_ossl = BN_new();
  BIGNUM *ry_ossl = BN_new();
  EC_POINT_get_affine_coordinates(group, R_ossl, rx_ossl, ry_ossl, ctx);
  
  print_bn("OpenSSL R.x", rx_ossl);
  print_bn("OpenSSL R.y", ry_ossl);
  
  /* Our implementation */
  fio___p384_scalar_s u1, u2;
  fio___p384_point_affine_s q;
  
  fio___p384_scalar_from_bytes(u1, u1_bytes);
  fio___p384_scalar_from_bytes(u2, u2_bytes);
  fio___p384_fe_from_bytes(q.x, qx_bytes);
  fio___p384_fe_from_bytes(q.y, qy_bytes);
  
  fio___p384_point_jacobian_s R_jac;
  fio___p384_point_mul2(&R_jac, u1, u2, &q);
  
  fio___p384_point_affine_s R_aff;
  fio___p384_point_to_affine(&R_aff, &R_jac);
  
  print_fe384("FIO R.x", R_aff.x);
  print_fe384("FIO R.y", R_aff.y);
  
  /* Compare */
  uint8_t rx_bytes[48], ry_bytes[48];
  fio___p384_fe_to_bytes(rx_bytes, R_aff.x);
  fio___p384_fe_to_bytes(ry_bytes, R_aff.y);
  
  uint8_t rx_ossl_bytes[48], ry_ossl_bytes[48];
  BN_bn2binpad(rx_ossl, rx_ossl_bytes, 48);
  BN_bn2binpad(ry_ossl, ry_ossl_bytes, 48);
  
  fprintf(stderr, "R.x match: %s\n", 
          FIO_MEMCMP(rx_bytes, rx_ossl_bytes, 48) == 0 ? "YES" : "NO");
  fprintf(stderr, "R.y match: %s\n", 
          FIO_MEMCMP(ry_bytes, ry_ossl_bytes, 48) == 0 ? "YES" : "NO");
  
  /* Now test individual components */
  fprintf(stderr, "\n--- Test: u1*G separately ---\n");
  
  EC_POINT *u1G_ossl = EC_POINT_new(group);
  EC_POINT_mul(group, u1G_ossl, u1_bn, NULL, NULL, ctx);
  
  BIGNUM *u1gx_ossl = BN_new();
  BIGNUM *u1gy_ossl = BN_new();
  EC_POINT_get_affine_coordinates(group, u1G_ossl, u1gx_ossl, u1gy_ossl, ctx);
  
  print_bn("OpenSSL u1*G.x", u1gx_ossl);
  print_bn("OpenSSL u1*G.y", u1gy_ossl);
  
  /* Our u1*G */
  fio___p384_point_jacobian_s u1G_jac;
  fio___p384_point_mul(&u1G_jac, u1, &g);
  fio___p384_point_affine_s u1G_aff;
  fio___p384_point_to_affine(&u1G_aff, &u1G_jac);
  
  print_fe384("FIO u1*G.x", u1G_aff.x);
  print_fe384("FIO u1*G.y", u1G_aff.y);
  
  uint8_t u1gx_bytes[48], u1gy_bytes[48];
  fio___p384_fe_to_bytes(u1gx_bytes, u1G_aff.x);
  fio___p384_fe_to_bytes(u1gy_bytes, u1G_aff.y);
  
  uint8_t u1gx_ossl_bytes[48], u1gy_ossl_bytes[48];
  BN_bn2binpad(u1gx_ossl, u1gx_ossl_bytes, 48);
  BN_bn2binpad(u1gy_ossl, u1gy_ossl_bytes, 48);
  
  fprintf(stderr, "u1*G.x match: %s\n", 
          FIO_MEMCMP(u1gx_bytes, u1gx_ossl_bytes, 48) == 0 ? "YES" : "NO");
  fprintf(stderr, "u1*G.y match: %s\n", 
          FIO_MEMCMP(u1gy_bytes, u1gy_ossl_bytes, 48) == 0 ? "YES" : "NO");
  
  fprintf(stderr, "\n--- Test: u2*Q separately ---\n");
  
  EC_POINT *u2Q_ossl = EC_POINT_new(group);
  EC_POINT_mul(group, u2Q_ossl, NULL, Q, u2_bn, ctx);
  
  BIGNUM *u2qx_ossl = BN_new();
  BIGNUM *u2qy_ossl = BN_new();
  EC_POINT_get_affine_coordinates(group, u2Q_ossl, u2qx_ossl, u2qy_ossl, ctx);
  
  print_bn("OpenSSL u2*Q.x", u2qx_ossl);
  print_bn("OpenSSL u2*Q.y", u2qy_ossl);
  
  /* Our u2*Q */
  fio___p384_point_jacobian_s u2Q_jac;
  fio___p384_point_mul(&u2Q_jac, u2, &q);
  fio___p384_point_affine_s u2Q_aff;
  fio___p384_point_to_affine(&u2Q_aff, &u2Q_jac);
  
  print_fe384("FIO u2*Q.x", u2Q_aff.x);
  print_fe384("FIO u2*Q.y", u2Q_aff.y);
  
  uint8_t u2qx_bytes[48], u2qy_bytes[48];
  fio___p384_fe_to_bytes(u2qx_bytes, u2Q_aff.x);
  fio___p384_fe_to_bytes(u2qy_bytes, u2Q_aff.y);
  
  uint8_t u2qx_ossl_bytes[48], u2qy_ossl_bytes[48];
  BN_bn2binpad(u2qx_ossl, u2qx_ossl_bytes, 48);
  BN_bn2binpad(u2qy_ossl, u2qy_ossl_bytes, 48);
  
  fprintf(stderr, "u2*Q.x match: %s\n", 
          FIO_MEMCMP(u2qx_bytes, u2qx_ossl_bytes, 48) == 0 ? "YES" : "NO");
  fprintf(stderr, "u2*Q.y match: %s\n", 
          FIO_MEMCMP(u2qy_bytes, u2qy_ossl_bytes, 48) == 0 ? "YES" : "NO");
  
  /* Test point addition: u1*G + u2*Q */
  fprintf(stderr, "\n--- Test: (u1*G) + (u2*Q) point addition ---\n");
  
  EC_POINT *sum_ossl = EC_POINT_new(group);
  EC_POINT_add(group, sum_ossl, u1G_ossl, u2Q_ossl, ctx);
  
  BIGNUM *sumx_ossl = BN_new();
  BIGNUM *sumy_ossl = BN_new();
  EC_POINT_get_affine_coordinates(group, sum_ossl, sumx_ossl, sumy_ossl, ctx);
  
  print_bn("OpenSSL (u1*G + u2*Q).x", sumx_ossl);
  print_bn("OpenSSL (u1*G + u2*Q).y", sumy_ossl);
  
  /* Our point addition */
  fio___p384_point_jacobian_s sum_jac;
  fio___p384_point_to_jacobian(&sum_jac, &u1G_aff);
  fio___p384_point_add_mixed(&sum_jac, &sum_jac, &u2Q_aff);
  
  fio___p384_point_affine_s sum_aff;
  fio___p384_point_to_affine(&sum_aff, &sum_jac);
  
  print_fe384("FIO (u1*G + u2*Q).x", sum_aff.x);
  print_fe384("FIO (u1*G + u2*Q).y", sum_aff.y);
  
  uint8_t sumx_bytes[48], sumy_bytes[48];
  fio___p384_fe_to_bytes(sumx_bytes, sum_aff.x);
  fio___p384_fe_to_bytes(sumy_bytes, sum_aff.y);
  
  uint8_t sumx_ossl_bytes[48], sumy_ossl_bytes[48];
  BN_bn2binpad(sumx_ossl, sumx_ossl_bytes, 48);
  BN_bn2binpad(sumy_ossl, sumy_ossl_bytes, 48);
  
  fprintf(stderr, "(u1*G + u2*Q).x match: %s\n", 
          FIO_MEMCMP(sumx_bytes, sumx_ossl_bytes, 48) == 0 ? "YES" : "NO");
  fprintf(stderr, "(u1*G + u2*Q).y match: %s\n", 
          FIO_MEMCMP(sumy_bytes, sumy_ossl_bytes, 48) == 0 ? "YES" : "NO");
  
  /* Cleanup */
  BN_free(u1_bn);
  BN_free(u2_bn);
  BN_free(qx_bn);
  BN_free(qy_bn);
  BN_free(rx_ossl);
  BN_free(ry_ossl);
  BN_free(u1gx_ossl);
  BN_free(u1gy_ossl);
  BN_free(u2qx_ossl);
  BN_free(u2qy_ossl);
  BN_free(sumx_ossl);
  BN_free(sumy_ossl);
  EC_POINT_free(Q);
  EC_POINT_free(R_ossl);
  EC_POINT_free(u1G_ossl);
  EC_POINT_free(u2Q_ossl);
  EC_POINT_free(sum_ossl);
  BN_CTX_free(ctx);
  EC_GROUP_free(group);
  
  fprintf(stderr, "\n=== OpenSSL comparison complete ===\n");
}

#endif /* HAVE_OPENSSL */

/* *****************************************************************************
Main Test Function
***************************************************************************** */
int main(int argc, char const *argv[]) {
  (void)argc;
  (void)argv;
  
#ifdef HAVE_OPENSSL
  test_scalar_mul_openssl();
#else
  fprintf(stderr, "OpenSSL not available, skipping comparison tests\n");
#endif
  
  return 0;
}
