/* *****************************************************************************
Compare fio P-256 point multiplication to OpenSSL.
***************************************************************************** */
#define FIO_LOG
#define FIO_TEST_CSTL
#define FIO_CRYPT
#define FIO_P256
#include "test-helpers.h"

#include <openssl/ec.h>
#include <openssl/bn.h>
#include <openssl/obj_mac.h>

static const uint8_t P256_G[65] = {
    0x04, 0x6b, 0x17, 0xd1, 0xf2, 0xe1, 0x2c, 0x42, 0x47, 0xf8, 0xbc, 0xe6,
    0xe5, 0x63, 0xa4, 0x40, 0xf2, 0x77, 0x03, 0x7d, 0x81, 0x2d, 0xeb, 0x33,
    0xa0, 0xf4, 0xa1, 0x39, 0x45, 0xd8, 0x98, 0xc2, 0x96, 0x4f, 0xe3, 0x42,
    0xe2, 0xfe, 0x1a, 0x7f, 0x9b, 0x8e, 0xe7, 0xeb, 0x4a, 0x7c, 0x0f, 0x9e,
    0x16, 0x2b, 0xce, 0x33, 0x57, 0x6b, 0x31, 0x5e, 0xce, 0xcb, 0xb6, 0x40,
    0x68, 0x37, 0xbf, 0x51, 0xf5};

/* OpenSSL 3.x deprecated the low-level EC_KEY API. There is no modern EVP API
 * that exposes arbitrary scalar-by-base-point multiplication, so this test
 * helper keeps using EC_KEY locally and silences the warning. */
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

FIO_SFUNC int p256_mul_openssl(const uint8_t k[32], uint8_t out[65]) {
  EC_KEY *key = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
  if (!key)
    return -1;
  const EC_GROUP *grp = EC_KEY_get0_group(key);
  EC_POINT *res = EC_POINT_new(grp);
  BIGNUM *bn = BN_bin2bn(k, 32, NULL);
  EC_POINT *G = EC_POINT_new(grp);
  if (!res || !bn || !G)
    goto fail;
  if (!EC_POINT_oct2point(grp, G, P256_G, 65, NULL))
    goto fail;
  if (!EC_POINT_mul(grp, res, NULL, G, bn, NULL))
    goto fail;
  size_t len = EC_POINT_point2oct(grp, res, POINT_CONVERSION_UNCOMPRESSED,
                                  out, 65, NULL);
  if (len != 65)
    goto fail;
  EC_POINT_free(G);
  EC_POINT_free(res);
  BN_free(bn);
  EC_KEY_free(key);
  return 0;
fail:
  if (G)
    EC_POINT_free(G);
  if (res)
    EC_POINT_free(res);
  if (bn)
    BN_free(bn);
  EC_KEY_free(key);
  return -1;
}

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

int main(void) {
  /* Test k = 1 */
  uint8_t k1[32] = {0};
  k1[31] = 1;
  fio___p256_scalar_s s1;
  fio___p256_scalar_from_bytes(s1, k1);
  fio___p256_point_affine_s g;
  fio___p256_fe_from_bytes(g.x, P256_G + 1);
  fio___p256_fe_from_bytes(g.y, P256_G + 33);
  fio___p256_point_jacobian_s R;
  fio___p256_point_mul(&R, s1, &g);
  fio___p256_point_affine_s A;
  fio___p256_point_to_affine(&A, &R);
  uint8_t fio_out[65];
  fio_out[0] = 0x04;
  fio___p256_fe_to_bytes(fio_out + 1, A.x);
  fio___p256_fe_to_bytes(fio_out + 33, A.y);

  uint8_t ssl_out[65];
  FIO_ASSERT(p256_mul_openssl(k1, ssl_out) == 0,
             "openssl k=1 mul should succeed");
  if (FIO_MEMCMP(fio_out, ssl_out, 65) != 0) {
    fprintf(stderr, "k=1 mismatch\nfio: ");
    for (size_t i = 0; i < 65; ++i)
      fprintf(stderr, "%02x", fio_out[i]);
    fprintf(stderr, "\nssl: ");
    for (size_t i = 0; i < 65; ++i)
      fprintf(stderr, "%02x", ssl_out[i]);
    fprintf(stderr, "\n");
  }
  FIO_ASSERT(FIO_MEMCMP(fio_out, ssl_out, 65) == 0,
             "fio k=1 point mul should match OpenSSL");

  /* Test k = 2 */
  uint8_t k2[32] = {0};
  k2[31] = 2;
  fio___p256_scalar_s s2;
  fio___p256_scalar_from_bytes(s2, k2);
  fio___p256_point_mul(&R, s2, &g);
  fio___p256_point_to_affine(&A, &R);
  fio_out[0] = 0x04;
  fio___p256_fe_to_bytes(fio_out + 1, A.x);
  fio___p256_fe_to_bytes(fio_out + 33, A.y);
  FIO_ASSERT(p256_mul_openssl(k2, ssl_out) == 0,
             "openssl k=2 mul should succeed");
  if (FIO_MEMCMP(fio_out, ssl_out, 65) != 0) {
    fprintf(stderr, "k=2 mismatch\nfio: ");
    for (size_t i = 0; i < 65; ++i)
      fprintf(stderr, "%02x", fio_out[i]);
    fprintf(stderr, "\nssl: ");
    for (size_t i = 0; i < 65; ++i)
      fprintf(stderr, "%02x", ssl_out[i]);
    fprintf(stderr, "\n");
  }
  FIO_ASSERT(FIO_MEMCMP(fio_out, ssl_out, 65) == 0,
             "fio k=2 point mul should match OpenSSL");

  return 0;
}
