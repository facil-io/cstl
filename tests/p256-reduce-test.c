/* *****************************************************************************
P-256 NIST Fast Reduction Debug Test
Test the fio___p256_fe_reduce function in isolation
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

FIO_SFUNC void print_512(const char *name, const uint64_t t[8]) {
  (void)name; /* Used only in debug logging */
  (void)t;    /* Used only in debug logging */
  FIO_LOG_DDEBUG("%s: %016llx%016llx%016llx%016llx%016llx%016llx%016llx%016llx",
                 name,
                 (unsigned long long)t[7],
                 (unsigned long long)t[6],
                 (unsigned long long)t[5],
                 (unsigned long long)t[4],
                 (unsigned long long)t[3],
                 (unsigned long long)t[2],
                 (unsigned long long)t[1],
                 (unsigned long long)t[0]);
}

/* Simple reduction using repeated subtraction - reference implementation */
FIO_SFUNC void simple_reduce(uint64_t r[4], const uint64_t t[8]) {
  /* Copy to larger buffer for subtraction */
  uint64_t tmp[9] = {t[0], t[1], t[2], t[3], t[4], t[5], t[6], t[7], 0};

  /* p in 64-bit limbs (extended to 8 limbs) */
  static const uint64_t p[8] = {0xFFFFFFFFFFFFFFFFULL,
                                0x00000000FFFFFFFFULL,
                                0x0000000000000000ULL,
                                0xFFFFFFFF00000001ULL,
                                0,
                                0,
                                0,
                                0};

  /* Subtract p * 2^(64*i) for each high limb */
  for (int shift = 4; shift >= 0; --shift) {
    while (1) {
      /* Check if we can subtract p * 2^(64*shift) */
      int can_subtract = 0;

      /* Compare from high to low */
      for (int i = 8; i >= shift; --i) {
        int ti = i;
        int pi = i - shift;
        if (pi < 0 || pi >= 4) {
          if (tmp[ti] > 0) {
            can_subtract = 1;
            break;
          }
        } else {
          if (tmp[ti] > p[pi]) {
            can_subtract = 1;
            break;
          } else if (tmp[ti] < p[pi]) {
            break;
          }
        }
      }

      if (!can_subtract)
        break;

      /* Subtract p * 2^(64*shift) */
      uint64_t borrow = 0;
      for (int i = 0; i < 4; ++i) {
        int ti = i + shift;
        if (ti > 8)
          break;
        uint64_t sub = p[i] + borrow;
        borrow = (sub < p[i]) ? 1 : 0;
        if (tmp[ti] < sub) {
          tmp[ti] = tmp[ti] - sub + (1ULL << 63) + (1ULL << 63);
          borrow = 1;
        } else {
          tmp[ti] -= sub;
        }
      }
      /* Propagate borrow */
      for (int i = shift + 4; i <= 8; ++i) {
        if (borrow == 0)
          break;
        if (tmp[i] >= borrow) {
          tmp[i] -= borrow;
          borrow = 0;
        } else {
          tmp[i] = tmp[i] - borrow + (1ULL << 63) + (1ULL << 63);
        }
      }
    }
  }

  r[0] = tmp[0];
  r[1] = tmp[1];
  r[2] = tmp[2];
  r[3] = tmp[3];
}

int main(void) {
  FIO_LOG_DDEBUG("=== P-256 NIST Fast Reduction Debug ===");

  /* Test 1: Reduce a small value (should be unchanged) */
  FIO_LOG_DDEBUG("=== Test 1: Small value (unchanged) ===");
  {
    uint64_t t[8] = {0x123456789ABCDEF0ULL,
                     0x0FEDCBA987654321ULL,
                     0x1111111111111111ULL,
                     0x2222222222222222ULL,
                     0,
                     0,
                     0,
                     0};
    fio___p256_fe_s result;
    fio___p256_fe_reduce(result, t);
    print_512("Input (512-bit)", t);
    print_fe("Output (256-bit)", result);
    FIO_LOG_DDEBUG("Expected: same as input low 256 bits");
  }

  /* Test 2: Reduce p itself (should become 0) */
  FIO_LOG_DDEBUG("=== Test 2: Reduce p (should be 0) ===");
  {
    uint64_t t[8] = {0xFFFFFFFFFFFFFFFFULL,
                     0x00000000FFFFFFFFULL,
                     0x0000000000000000ULL,
                     0xFFFFFFFF00000001ULL,
                     0,
                     0,
                     0,
                     0};
    fio___p256_fe_s result;
    fio___p256_fe_reduce(result, t);
    print_512("Input (p)", t);
    print_fe("Output", result);
    FIO_LOG_DDEBUG("Expected: 0");
  }

  /* Test 3: Reduce 2*p (should be 0) */
  FIO_LOG_DDEBUG("=== Test 3: Reduce 2*p (should be 0) ===");
  {
    /* 2*p = 0x1fffffffe00000002000000000000000000000001fffffffffffffffffffffffe
     */
    /* Correct values from Ruby verification: */
    uint64_t t[8] = {0xFFFFFFFFFFFFFFFEULL, /* t[0] */
                     0x00000001FFFFFFFFULL, /* t[1] */
                     0x0000000000000000ULL, /* t[2] - was incorrectly 1 */
                     0xFFFFFFFE00000002ULL, /* t[3] */
                     0x0000000000000001ULL, /* t[4] */
                     0,
                     0,
                     0};
    fio___p256_fe_s result;
    fio___p256_fe_reduce(result, t);
    print_512("Input (2*p)", t);
    print_fe("Output", result);
    FIO_LOG_DDEBUG("Expected: 0");
  }

  /* Test 4: Compute G.x^2 mod p using fe_sqr and verify */
  FIO_LOG_DDEBUG("=== Test 4: G.x^2 mod p ===");
  {
    fio___p256_fe_s gx, gx_sqr;
    fio___p256_fe_copy(gx, FIO___P256_GX);
    fio___p256_fe_sqr(gx_sqr, gx);
    print_fe("G.x", gx);
    print_fe("G.x^2 mod p", gx_sqr);

    /* Verify by computing G.x * G.x */
    fio___p256_fe_s gx_mul;
    fio___p256_fe_mul(gx_mul, gx, gx);
    print_fe("G.x * G.x mod p", gx_mul);

    if (fio___p256_fe_eq(gx_sqr, gx_mul) == 0) {
      FIO_LOG_DDEBUG("sqr == mul: OK");
    } else {
      FIO_LOG_DDEBUG("sqr != mul: FAIL");
    }
  }

  /* Test 5: Verify a * a^(-1) = 1 */
  FIO_LOG_DDEBUG("=== Test 5: G.x * G.x^(-1) = 1 ===");
  {
    fio___p256_fe_s gx, gx_inv, result;
    fio___p256_fe_copy(gx, FIO___P256_GX);
    fio___p256_fe_inv(gx_inv, gx);
    fio___p256_fe_mul(result, gx, gx_inv);
    print_fe("G.x", gx);
    print_fe("G.x^(-1)", gx_inv);
    print_fe("G.x * G.x^(-1)", result);

    fio___p256_fe_s one;
    fio___p256_fe_one(one);
    if (fio___p256_fe_eq(result, one) == 0) {
      FIO_LOG_DDEBUG("G.x * G.x^(-1) == 1: OK");
    } else {
      FIO_LOG_DDEBUG("G.x * G.x^(-1) != 1: FAIL");
    }
  }

  /* Test 6: Known multiplication result from OpenSSL/reference */
  FIO_LOG_DDEBUG("=== Test 6: Specific 512-bit reduction test ===");
  {
    /* Compute G.x * G.y to get a 512-bit product, then reduce */
    /* First, let's manually compute the product */
    uint64_t gx[4], gy[4];
    for (int i = 0; i < 4; i++) {
      gx[i] = FIO___P256_GX[i];
      gy[i] = FIO___P256_GY[i];
    }

    /* Schoolbook multiplication to get 512-bit product */
    uint64_t t[8] = {0};
    for (int i = 0; i < 4; ++i) {
      uint64_t carry = 0;
      for (int j = 0; j < 4; ++j) {
        uint64_t hi;
        uint64_t lo = fio_math_mulc64(gx[i], gy[j], &hi);
        uint64_t c1 = 0, c2 = 0;
        t[i + j] = fio_math_addc64(t[i + j], lo, 0, &c1);
        t[i + j] = fio_math_addc64(t[i + j], carry, 0, &c2);
        carry = hi + c1 + c2;
      }
      t[i + 4] += carry;
    }

    print_512("G.x * G.y (512-bit)", t);

    /* Reduce using the NIST formula */
    fio___p256_fe_s result;
    fio___p256_fe_reduce(result, t);
    print_fe("After NIST reduce", result);

    /* Compare with fe_mul result */
    fio___p256_fe_s expected;
    fio___p256_fe_mul(expected, FIO___P256_GX, FIO___P256_GY);
    print_fe("fe_mul result", expected);

    if (fio___p256_fe_eq(result, expected) == 0) {
      FIO_LOG_DDEBUG("NIST reduce == fe_mul: OK");
    } else {
      FIO_LOG_DDEBUG("NIST reduce != fe_mul: FAIL");
    }
  }

  /* Test 7: Test with max values (close to overflow) */
  FIO_LOG_DDEBUG("=== Test 7: Large values near 2^512 ===");
  {
    uint64_t t[8] = {0xFFFFFFFFFFFFFFFFULL,
                     0xFFFFFFFFFFFFFFFFULL,
                     0xFFFFFFFFFFFFFFFFULL,
                     0xFFFFFFFFFFFFFFFFULL,
                     0xFFFFFFFFFFFFFFFFULL,
                     0xFFFFFFFFFFFFFFFFULL,
                     0xFFFFFFFFFFFFFFFFULL,
                     0xFFFFFFFFFFFFFFFFULL};
    fio___p256_fe_s result;
    fio___p256_fe_reduce(result, t);
    print_512("Input (max 512-bit)", t);
    print_fe("After reduce", result);

    /* Verify result is less than p */
    int valid = 1;
    for (int i = 3; i >= 0; --i) {
      if (result[i] > FIO___P256_P[i]) {
        valid = 0;
        break;
      } else if (result[i] < FIO___P256_P[i]) {
        break;
      }
    }
    (void)valid; /* Used only in debug logging */
    FIO_LOG_DDEBUG("Result < p: %s", valid ? "OK" : "FAIL");
  }

  /* Test 8: Trace through one step of scalar multiplication */
  FIO_LOG_DDEBUG("=== Test 8: Trace d*G step by step ===");
  {
    /* The failing scalar d */
    static const uint8_t d_bytes[32] = {
        0xc9, 0xaf, 0xa9, 0xd8, 0x45, 0xba, 0x75, 0x16, 0x6b, 0x5c, 0x21,
        0x57, 0x67, 0xb1, 0xd6, 0x93, 0x4e, 0x50, 0xc3, 0xdb, 0x36, 0xe8,
        0x9b, 0x12, 0x7b, 0x8a, 0x62, 0x2b, 0x12, 0x0f, 0x67, 0x21};
    fio___p256_scalar_s d;
    fio___p256_scalar_from_bytes(d, d_bytes);

    /* Base point G */
    fio___p256_point_affine_s g;
    fio___p256_fe_copy(g.x, FIO___P256_GX);
    fio___p256_fe_copy(g.y, FIO___P256_GY);

    /* Start with just the top byte (0xc9 = 201 in decimal) */
    /* That's bits 255-248 */
    /* 0xc9 = 11001001 in binary */
    /* So we start at bit 255 (which is 1), then double, bit 254 (1), double,
       bit 253 (0), ... */

    FIO_LOG_DDEBUG("d = 0x%016llx%016llx%016llx%016llx",
                   (unsigned long long)d[3],
                   (unsigned long long)d[2],
                   (unsigned long long)d[1],
                   (unsigned long long)d[0]);

    /* Find highest bit */
    int start_bit = 255;
    while (start_bit >= 0) {
      int limb = start_bit / 64;
      int bit = start_bit % 64;
      if (d[limb] & (1ULL << bit))
        break;
      --start_bit;
    }
    FIO_LOG_DDEBUG("Start bit: %d", start_bit);

    /* Do just the first 8 iterations and trace */
    fio___p256_point_jacobian_s acc;
    fio___p256_point_set_infinity(&acc);

    for (int i = start_bit; i >= start_bit - 7 && i >= 0; --i) {
      fio___p256_point_double(&acc, &acc);

      int limb = i / 64;
      int bit = i % 64;
      int b = (d[limb] >> bit) & 1;

      if (b) {
        fio___p256_point_add_mixed(&acc, &acc, &g);
      }

      /* Convert to affine for display */
      fio___p256_point_affine_s aff;
      fio___p256_point_to_affine(&aff, &acc);
      FIO_LOG_DDEBUG("  bit %d = %d, acc.x = %016llx...",
                     i,
                     b,
                     (unsigned long long)aff.x[3]);
    }
  }

  return 0;
}
