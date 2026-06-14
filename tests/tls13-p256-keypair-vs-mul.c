/* *****************************************************************************
Compare fio_p256_keypair to fio point_mul for same secret.
***************************************************************************** */
#define FIO_LOG
#define FIO_TEST_CSTL
#define FIO_CRYPT
#define FIO_P256
#include "test-helpers.h"

int main(void) {
  uint8_t secret[32] = {
      0xe1, 0x28, 0x33, 0xcf, 0xbf, 0x0d, 0xda, 0x08, 0x70, 0x79, 0x4e,
      0xed, 0x00, 0x3a, 0xe9, 0xe9, 0xd8, 0x56, 0x29, 0xea, 0x57, 0xa5,
      0xe0, 0x3a, 0x43, 0xd7, 0xb1, 0x42, 0xa7, 0xfc, 0x9e, 0x10};

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
    fprintf(stderr, "pub1: ");
    for (size_t i = 0; i < 65; ++i)
      fprintf(stderr, "%02x", pub1[i]);
    fprintf(stderr, "\npub2: ");
    for (size_t i = 0; i < 65; ++i)
      fprintf(stderr, "%02x", pub2[i]);
    fprintf(stderr, "\n");
  }
  FIO_ASSERT(FIO_MEMCMP(pub1, pub2, 65) == 0,
             "fio_p256_keypair should match point_mul");
  return 0;
}
