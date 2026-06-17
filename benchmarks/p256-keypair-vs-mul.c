/* *****************************************************************************
P-256 keypair vs point_mul comparison benchmark (154 p256.h)

Ported from tests-old/tls13-p256-keypair-vs-mul.c.
Compares fio_p256_keypair output against a manual scalar-by-generator
multiplication using fio___p256_point_mul.  No external libraries are used.
***************************************************************************** */
#define FIO_LOG
#define FIO_TEST_CSTL
#define FIO_CRYPT
#define FIO_P256
#include "tests/test-helpers.h"

int main(void) {
  const size_t rounds = 64;
  uint8_t secret[32] = {
      0xe1, 0x28, 0x33, 0xcf, 0xbf, 0x0d, 0xda, 0x08, 0x70, 0x79, 0x4e,
      0xed, 0x00, 0x3a, 0xe9, 0xe9, 0xd8, 0x56, 0x29, 0xea, 0x57, 0xa5,
      0xe0, 0x3a, 0x43, 0xd7, 0xb1, 0x42, 0xa7, 0xfc, 0x9e, 0x10};

  fprintf(stderr,
          "P-256 keypair vs point_mul (%zu deterministic + random secrets)...\n",
          rounds);
  int64_t start = FIO_NAME_TEST(stl, atol_time)();

  for (size_t r = 0; r < rounds; ++r) {
    uint8_t pub1[65];
    FIO_ASSERT(fio_p256_keypair(secret, pub1) == 0,
               "fio_p256_keypair should succeed");

    fio___p256_scalar_s k;
    fio___p256_scalar_from_bytes(k, secret);
    fio___p256_point_affine_s g;
    fio___p256_fe_copy(g.x, FIO___P256_GX);
    fio___p256_fe_copy(g.y, FIO___P256_GY);
    fio___p256_point_jacobian_s R;
    fio___p256_point_mul(&R, k, &g);
    fio___p256_point_affine_s A;
    fio___p256_point_to_affine(&A, &R);
    uint8_t pub2[65];
    pub2[0] = 0x04;
    fio___p256_fe_to_bytes(pub2 + 1, A.x);
    fio___p256_fe_to_bytes(pub2 + 33, A.y);

    if (FIO_MEMCMP(pub1, pub2, 65) != 0) {
      fprintf(stderr, "mismatch on round %zu\n", r);
      fprintf(stderr, "secret: ");
      for (size_t i = 0; i < 32; ++i)
        fprintf(stderr, "%02x", secret[i]);
      fprintf(stderr, "\npub1: ");
      for (size_t i = 0; i < 65; ++i)
        fprintf(stderr, "%02x", pub1[i]);
      fprintf(stderr, "\npub2: ");
      for (size_t i = 0; i < 65; ++i)
        fprintf(stderr, "%02x", pub2[i]);
      fprintf(stderr, "\n");
    }
    FIO_ASSERT(FIO_MEMCMP(pub1, pub2, 65) == 0,
               "fio_p256_keypair should match point_mul");

    /* rotate to a new random secret for the next round */
    fio_rand_bytes(secret, 32);
    secret[0] &= 0x7F;
  }

  fprintf(stderr,
          "  keypair vs point_mul: %zu rounds in %lld us\n",
          rounds,
          (long long)(FIO_NAME_TEST(stl, atol_time)() - start));
  fprintf(stderr, "P-256 keypair vs point_mul benchmark passed\n");
  return 0;
}
