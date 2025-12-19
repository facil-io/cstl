/* *****************************************************************************
P-256 Step-by-Step Debug Test
***************************************************************************** */
#include "test-helpers.h"

#define FIO_P256
#define FIO_SHA2
#define FIO_LOG
#include FIO_INCLUDE_FILE

FIO_SFUNC void print_fe(const char *name, const uint64_t fe[4]) {
  (void)name; /* Used only in debug logging */
  (void)fe;   /* Used only in debug logging */
  FIO_LOG_DDEBUG("%s: %016llx%016llx%016llx%016llx",
                 name,
                 (unsigned long long)fe[3],
                 (unsigned long long)fe[2],
                 (unsigned long long)fe[1],
                 (unsigned long long)fe[0]);
}

/* Test: compute k*G step by step and verify intermediate results */
int main(void) {
  FIO_LOG_DDEBUG("=== P-256 Step-by-Step Debug ===");

  /* Base point G */
  fio___p256_point_affine_s g;
  fio___p256_fe_copy(g.x, FIO___P256_GX);
  fio___p256_fe_copy(g.y, FIO___P256_GY);

  /* Compute multiples of G by repeated addition */
  fio___p256_point_jacobian_s acc;
  fio___p256_point_to_jacobian(&acc, &g); /* acc = 1*G */

  FIO_LOG_DDEBUG("=== Small multiples k*G ===");
  for (int k = 2; k <= 16; k++) {
    fio___p256_point_add_mixed(&acc, &acc, &g); /* acc = k*G */

    fio___p256_point_affine_s acc_aff;
    fio___p256_point_to_affine(&acc_aff, &acc);

    /* Also compute k*G via scalar multiplication */
    fio___p256_scalar_s scalar = {(uint64_t)k, 0, 0, 0};
    fio___p256_point_jacobian_s scalar_result;
    fio___p256_point_mul(&scalar_result, scalar, &g);
    fio___p256_point_affine_s scalar_aff;
    fio___p256_point_to_affine(&scalar_aff, &scalar_result);

    /* Check if they match */
    int match = (fio___p256_fe_eq(acc_aff.x, scalar_aff.x) == 0 &&
                 fio___p256_fe_eq(acc_aff.y, scalar_aff.y) == 0);
    (void)match; /* Used only in debug logging */

    FIO_LOG_DDEBUG("%2d*G: add=%016llx... scalar=%016llx... %s",
                   k,
                   (unsigned long long)acc_aff.x[3],
                   (unsigned long long)scalar_aff.x[3],
                   match ? "OK" : "MISMATCH!");
  }

  /* Now test larger scalars - powers of 2 */
  FIO_LOG_DDEBUG("=== Powers of 2 ===");
  for (int exp = 8; exp <= 200; exp += 8) {
    /* Compute 2^exp * G by repeated doubling */
    fio___p256_point_jacobian_s doubled;
    fio___p256_point_to_jacobian(&doubled, &g);
    for (int i = 0; i < exp; i++) {
      fio___p256_point_double(&doubled, &doubled);
    }
    fio___p256_point_affine_s doubled_aff;
    fio___p256_point_to_affine(&doubled_aff, &doubled);

    /* Compute via scalar multiplication */
    fio___p256_scalar_s scalar = {0, 0, 0, 0};
    int limb = exp / 64;
    int bit = exp % 64;
    if (limb < 4)
      scalar[limb] = 1ULL << bit;

    fio___p256_point_jacobian_s scalar_result;
    fio___p256_point_mul(&scalar_result, scalar, &g);
    fio___p256_point_affine_s scalar_aff;
    fio___p256_point_to_affine(&scalar_aff, &scalar_result);

    /* Check if they match */
    int match = (fio___p256_fe_eq(doubled_aff.x, scalar_aff.x) == 0 &&
                 fio___p256_fe_eq(doubled_aff.y, scalar_aff.y) == 0);

    FIO_LOG_DDEBUG("2^%3d*G: double=%016llx... scalar=%016llx... %s",
                   exp,
                   (unsigned long long)doubled_aff.x[3],
                   (unsigned long long)scalar_aff.x[3],
                   match ? "OK" : "MISMATCH!");

    if (!match) {
      print_fe("  doubled.x", doubled_aff.x);
      print_fe("  scalar.x ", scalar_aff.x);
      break; /* Stop at first mismatch */
    }
  }

  /* Test: compute 0x100 * G (256) step by step */
  FIO_LOG_DDEBUG("=== 256*G (0x100) step by step ===");
  {
    /* Method 1: scalar mul */
    fio___p256_scalar_s k256 = {256, 0, 0, 0};
    fio___p256_point_jacobian_s result1;
    fio___p256_point_mul(&result1, k256, &g);
    fio___p256_point_affine_s aff1;
    fio___p256_point_to_affine(&aff1, &result1);

    /* Method 2: 256 additions */
    fio___p256_point_jacobian_s acc2;
    fio___p256_point_to_jacobian(&acc2, &g);
    for (int i = 1; i < 256; i++) {
      fio___p256_point_add_mixed(&acc2, &acc2, &g);
    }
    fio___p256_point_affine_s aff2;
    fio___p256_point_to_affine(&aff2, &acc2);

    /* Method 3: 8 doubles (2^8 = 256) */
    fio___p256_point_jacobian_s acc3;
    fio___p256_point_to_jacobian(&acc3, &g);
    for (int i = 0; i < 8; i++) {
      fio___p256_point_double(&acc3, &acc3);
    }
    fio___p256_point_affine_s aff3;
    fio___p256_point_to_affine(&aff3, &acc3);

    print_fe("256*G scalar mul", aff1.x);
    print_fe("256*G additions ", aff2.x);
    print_fe("256*G 8 doubles ", aff3.x);

    int match12 = fio___p256_fe_eq(aff1.x, aff2.x) == 0;
    int match13 = fio___p256_fe_eq(aff1.x, aff3.x) == 0;
    int match23 = fio___p256_fe_eq(aff2.x, aff3.x) == 0;
    (void)match12; /* Used only in debug logging */
    (void)match13; /* Used only in debug logging */
    (void)match23; /* Used only in debug logging */

    FIO_LOG_DDEBUG("scalar==add: %s, scalar==double: %s, add==double: %s",
                   match12 ? "OK" : "FAIL",
                   match13 ? "OK" : "FAIL",
                   match23 ? "OK" : "FAIL");
  }

  /* Test with the specific failing scalar d */
  FIO_LOG_DDEBUG("=== Test d*G ===");
  static const uint8_t d_bytes[32] = {
      0xc9, 0xaf, 0xa9, 0xd8, 0x45, 0xba, 0x75, 0x16, 0x6b, 0x5c, 0x21,
      0x57, 0x67, 0xb1, 0xd6, 0x93, 0x4e, 0x50, 0xc3, 0xdb, 0x36, 0xe8,
      0x9b, 0x12, 0x7b, 0x8a, 0x62, 0x2b, 0x12, 0x0f, 0x67, 0x21};

  /* Expected Q = d*G from RFC 6979 A.2.5 (P-256 with SHA-256)
   * Ux = 60FED4BA255A9D31C961EB74C6356D68C049B8923B61FA6CE669622E60F29FB6
   */
  static const uint8_t expected_x[32] = {
      0x60, 0xfe, 0xd4, 0xba, 0x25, 0x5a, 0x9d, 0x31, 0xc9, 0x61, 0xeb,
      0x74, 0xc6, 0x35, 0x6d, 0x68, 0xc0, 0x49, 0xb8, 0x92, 0x3b, 0x61,
      0xfa, 0x6c, 0xe6, 0x69, 0x62, 0x2e, 0x60, 0xf2, 0x9f, 0xb6};

  fio___p256_scalar_s d;
  fio___p256_scalar_from_bytes(d, d_bytes);

  /* Compute d*G via scalar multiplication */
  fio___p256_point_jacobian_s dG;
  fio___p256_point_mul(&dG, d, &g);
  fio___p256_point_affine_s dG_aff;
  fio___p256_point_to_affine(&dG_aff, &dG);

  /* Load expected */
  fio___p256_fe_s exp_x;
  fio___p256_fe_from_bytes(exp_x, expected_x);

  print_fe("d*G.x (computed)", dG_aff.x);
  print_fe("d*G.x (expected)", exp_x);

  if (fio___p256_fe_eq(dG_aff.x, exp_x) == 0) {
    FIO_LOG_DDEBUG("d*G: PASS");
  } else {
    FIO_LOG_DDEBUG("d*G: FAIL");
  }

  return 0;
}
