/* *****************************************************************************
P-256 Binary Decomposition Debug Test
Find exactly where scalar multiplication diverges from expected
***************************************************************************** */
#include "test-helpers.h"

#define FIO_P256
#define FIO_SHA2
#define FIO_LOG
#include FIO_INCLUDE_FILE

FIO_SFUNC void print_fe(const char *name, const uint64_t fe[4]) {
  fprintf(stderr, "%s: ", name);
  for (int i = 3; i >= 0; --i)
    fprintf(stderr, "%016llx", (unsigned long long)fe[i]);
  fprintf(stderr, "\n");
}

/* Compute k*G using manual double-and-add, step by step */
FIO_SFUNC void manual_scalar_mul(fio___p256_point_affine_s *result,
                                 const fio___p256_scalar_s k,
                                 const fio___p256_point_affine_s *g) {
  fio___p256_point_jacobian_s acc;
  fio___p256_point_set_infinity(&acc);

  /* Find highest bit */
  int start_bit = 255;
  while (start_bit >= 0) {
    int limb = start_bit / 64;
    int bit = start_bit % 64;
    if (k[limb] & (1ULL << bit))
      break;
    --start_bit;
  }

  if (start_bit < 0) {
    fio___p256_fe_zero(result->x);
    fio___p256_fe_zero(result->y);
    return;
  }

  /* Double-and-add from MSB to LSB */
  for (int i = start_bit; i >= 0; --i) {
    fio___p256_point_double(&acc, &acc);

    int limb = i / 64;
    int bit = i % 64;
    if (k[limb] & (1ULL << bit)) {
      fio___p256_point_add_mixed(&acc, &acc, g);
    }
  }

  fio___p256_point_to_affine(result, &acc);
}

int main(void) {
  fprintf(stderr, "=== P-256 Binary Decomposition Debug ===\n\n");

  /* Base point G */
  fio___p256_point_affine_s g;
  fio___p256_fe_copy(g.x, FIO___P256_GX);
  fio___p256_fe_copy(g.y, FIO___P256_GY);

  /* The failing scalar d */
  static const uint8_t d_bytes[32] = {
      0xc9, 0xaf, 0xa9, 0xd8, 0x45, 0xba, 0x75, 0x16, 0x6b, 0x5c, 0x21,
      0x57, 0x67, 0xb1, 0xd6, 0x93, 0x4e, 0x50, 0xc3, 0xdb, 0x36, 0xe8,
      0x9b, 0x12, 0x7b, 0x8a, 0x62, 0x2b, 0x12, 0x0f, 0x67, 0x21};
  fio___p256_scalar_s d;
  fio___p256_scalar_from_bytes(d, d_bytes);

  fprintf(stderr,
          "d = %016llx%016llx%016llx%016llx\n",
          (unsigned long long)d[3],
          (unsigned long long)d[2],
          (unsigned long long)d[1],
          (unsigned long long)d[0]);

  /* Test partial scalars - take first N bits of d */
  fprintf(stderr, "\n=== Partial scalar tests (first N bits of d) ===\n");

  int all_match = 1;
  for (int nbits = 8; nbits <= 256; nbits += 8) {
    /* Create partial scalar with first nbits of d */
    fio___p256_scalar_s partial = {0, 0, 0, 0};

    /* Copy only the top nbits */
    /* d is 256 bits, so bit 255 is MSB */
    for (int i = 0; i < nbits; i++) {
      int src_bit = 255 - i; /* Source bit in d (MSB first) */
      int dst_bit = 255 - i; /* Destination bit in partial */

      int src_limb = src_bit / 64;
      int src_pos = src_bit % 64;
      int dst_limb = dst_bit / 64;
      int dst_pos = dst_bit % 64;

      if (d[src_limb] & (1ULL << src_pos)) {
        partial[dst_limb] |= (1ULL << dst_pos);
      }
    }

    /* Compute partial*G using library function */
    fio___p256_point_jacobian_s result_lib;
    fio___p256_point_mul(&result_lib, partial, &g);
    fio___p256_point_affine_s aff_lib;
    fio___p256_point_to_affine(&aff_lib, &result_lib);

    /* Compute partial*G using manual double-and-add */
    fio___p256_point_affine_s aff_manual;
    manual_scalar_mul(&aff_manual, partial, &g);

    /* Check if they match */
    int match = (fio___p256_fe_eq(aff_lib.x, aff_manual.x) == 0 &&
                 fio___p256_fe_eq(aff_lib.y, aff_manual.y) == 0);

    if (!match) {
      fprintf(stderr, "FIRST MISMATCH at %d bits!\n", nbits);
      fprintf(stderr,
              "partial = %016llx%016llx%016llx%016llx\n",
              (unsigned long long)partial[3],
              (unsigned long long)partial[2],
              (unsigned long long)partial[1],
              (unsigned long long)partial[0]);
      print_fe("  library.x", aff_lib.x);
      print_fe("  manual.x ", aff_manual.x);
      all_match = 0;
      break;
    }

    /* Every 32 bits, print progress */
    if (nbits % 32 == 0) {
      fprintf(stderr,
              "%3d bits: lib=%016llx... manual=%016llx... OK\n",
              nbits,
              (unsigned long long)aff_lib.x[3],
              (unsigned long long)aff_manual.x[3]);
    }
  }

  if (all_match) {
    fprintf(stderr, "\nAll partial scalars match!\n");
    fprintf(stderr, "This means manual_scalar_mul == fio___p256_point_mul\n");
    fprintf(
        stderr,
        "The bug is NOT in point_mul - both produce the same wrong answer.\n");
    fprintf(stderr, "\nThe bug must be in:\n");
    fprintf(stderr, "  1. Point doubling (fio___p256_point_double)\n");
    fprintf(stderr, "  2. Point addition (fio___p256_point_add_mixed)\n");
    fprintf(stderr, "  3. Field arithmetic (fio___p256_fe_*)\n");
    fprintf(stderr, "  4. Affine conversion (fio___p256_point_to_affine)\n");

    /* Let's verify the expected Q is on the curve */
    fprintf(stderr, "\n=== Verify expected Q is on curve ===\n");
    /* Expected Q = d*G from RFC 6979 A.2.5 (P-256 with SHA-256)
     * Ux = 60FED4BA255A9D31C961EB74C6356D68C049B8923B61FA6CE669622E60F29FB6
     * Uy = 7903FE1008B8BC99A41AE9E95628BC64F2F1B20C2D7E9F5177A3C294D4462299
     */
    static const uint8_t expected_x[32] = {
        0x60, 0xfe, 0xd4, 0xba, 0x25, 0x5a, 0x9d, 0x31, 0xc9, 0x61, 0xeb,
        0x74, 0xc6, 0x35, 0x6d, 0x68, 0xc0, 0x49, 0xb8, 0x92, 0x3b, 0x61,
        0xfa, 0x6c, 0xe6, 0x69, 0x62, 0x2e, 0x60, 0xf2, 0x9f, 0xb6};
    static const uint8_t expected_y[32] = {
        0x79, 0x03, 0xfe, 0x10, 0x08, 0xb8, 0xbc, 0x99, 0xa4, 0x1a, 0xe9,
        0xe9, 0x56, 0x28, 0xbc, 0x64, 0xf2, 0xf1, 0xb2, 0x0c, 0x2d, 0x7e,
        0x9f, 0x51, 0x77, 0xa3, 0xc2, 0x94, 0xd4, 0x46, 0x22, 0x99};

    fio___p256_fe_s qx, qy;
    fio___p256_fe_from_bytes(qx, expected_x);
    fio___p256_fe_from_bytes(qy, expected_y);

    /* Check y² = x³ - 3x + b */
    fio___p256_fe_s y2, x3, t;
    fio___p256_fe_sqr(y2, qy);
    fio___p256_fe_sqr(t, qx);
    fio___p256_fe_mul(x3, t, qx);
    fio___p256_fe_sub(t, x3, qx);
    fio___p256_fe_sub(t, t, qx);
    fio___p256_fe_sub(t, t, qx);
    fio___p256_fe_add(t, t, FIO___P256_B);

    if (fio___p256_fe_eq(y2, t) == 0) {
      fprintf(stderr, "Expected Q IS on the curve.\n");
    } else {
      fprintf(stderr,
              "ERROR: Expected Q is NOT on the curve! Test vector is wrong.\n");
    }

    /* Compute d*G and check if result is on curve */
    fio___p256_point_jacobian_s dG;
    fio___p256_point_mul(&dG, d, &g);
    fio___p256_point_affine_s dG_aff;
    fio___p256_point_to_affine(&dG_aff, &dG);

    fio___p256_fe_sqr(y2, dG_aff.y);
    fio___p256_fe_sqr(t, dG_aff.x);
    fio___p256_fe_mul(x3, t, dG_aff.x);
    fio___p256_fe_sub(t, x3, dG_aff.x);
    fio___p256_fe_sub(t, t, dG_aff.x);
    fio___p256_fe_sub(t, t, dG_aff.x);
    fio___p256_fe_add(t, t, FIO___P256_B);

    if (fio___p256_fe_eq(y2, t) == 0) {
      fprintf(stderr, "Computed d*G IS on the curve.\n");
    } else {
      fprintf(stderr, "ERROR: Computed d*G is NOT on the curve!\n");
    }

    print_fe("Computed d*G.x", dG_aff.x);
    print_fe("Expected Q.x  ", qx);
  }

  return 0;
}
