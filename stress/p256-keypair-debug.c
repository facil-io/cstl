/* *****************************************************************************
P-256 keypair debug stress harness (154 p256.h)

Ported from tests-old/tls13-p256-keypair-debug.c.
Runs multiple random secrets and prints the secret plus the three derived
public keys (fio_p256_keypair, manual point_mul, and OpenSSL) so any mismatch
is easy to inspect.  This is intended for manual debugging rather than default
CI.

No external processes are spawned; OpenSSL is used through libcrypto only.
***************************************************************************** */
#define FIO_LOG
#define FIO_TEST_CSTL
#define FIO_CRYPT
#define FIO_P256
#include "tests/test-helpers.h"

#include <openssl/ec.h>
#include <openssl/bn.h>
#include <openssl/obj_mac.h>

/* OpenSSL 3.x deprecated the low-level EC_KEY API.  There is no modern EVP API
 * that exposes arbitrary scalar-by-base-point multiplication, so this harness
 * keeps using EC_KEY locally and silences the warning. */
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

FIO_SFUNC int p256_public_from_secret_openssl(const uint8_t secret[32],
                                              uint8_t pub[65]) {
  EC_KEY *key = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
  if (!key)
    return -1;
  BIGNUM *bn = BN_bin2bn(secret, 32, NULL);
  EC_POINT *pub_pt = NULL;
  if (!bn)
    goto fail;
  const EC_GROUP *grp = EC_KEY_get0_group(key);
  pub_pt = EC_POINT_new(grp);
  if (!pub_pt)
    goto fail;
  if (!EC_POINT_mul(grp, pub_pt, bn, NULL, NULL, NULL))
    goto fail;
  size_t len = EC_POINT_point2oct(grp, pub_pt, POINT_CONVERSION_UNCOMPRESSED,
                                  pub, 65, NULL);
  if (len != 65)
    goto fail;
  EC_POINT_free(pub_pt);
  BN_free(bn);
  EC_KEY_free(key);
  return 0;
fail:
  if (pub_pt)
    EC_POINT_free(pub_pt);
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
  const size_t rounds = 32;
  fprintf(stderr, "P-256 keypair debug stress (%zu random secrets)...\n", rounds);

  for (size_t r = 0; r < rounds; ++r) {
    uint8_t secret[32];
    fio_rand_bytes(secret, 32);
    secret[0] &= 0x7f;

    uint8_t fio_pub[65];
    FIO_ASSERT(fio_p256_keypair(secret, fio_pub) == 0,
               "fio_p256_keypair should succeed");

    uint8_t openssl_pub[65];
    FIO_ASSERT(p256_public_from_secret_openssl(secret, openssl_pub) == 0,
               "openssl derivation should succeed");

    fio___p256_scalar_s k;
    fio___p256_scalar_from_bytes(k, secret);
    fio___p256_point_affine_s g;
    fio___p256_fe_copy(g.x, FIO___P256_GX);
    fio___p256_fe_copy(g.y, FIO___P256_GY);
    fio___p256_point_jacobian_s R;
    fio___p256_point_mul(&R, k, &g);
    fio___p256_point_affine_s A;
    fio___p256_point_to_affine(&A, &R);
    uint8_t mul_pub[65];
    mul_pub[0] = 0x04;
    fio___p256_fe_to_bytes(mul_pub + 1, A.x);
    fio___p256_fe_to_bytes(mul_pub + 33, A.y);

    fprintf(stderr, "round %zu\n", r);
    fprintf(stderr, "secret: ");
    for (size_t i = 0; i < 32; ++i)
      fprintf(stderr, "%02x", secret[i]);
    fprintf(stderr, "\nfio_pub: ");
    for (size_t i = 0; i < 65; ++i)
      fprintf(stderr, "%02x", fio_pub[i]);
    fprintf(stderr, "\nmul_pub: ");
    for (size_t i = 0; i < 65; ++i)
      fprintf(stderr, "%02x", mul_pub[i]);
    fprintf(stderr, "\nssl_pub: ");
    for (size_t i = 0; i < 65; ++i)
      fprintf(stderr, "%02x", openssl_pub[i]);
    fprintf(stderr, "\n");

    FIO_ASSERT(FIO_MEMCMP(fio_pub, openssl_pub, 65) == 0,
               "fio_p256_keypair should match openssl");
    FIO_ASSERT(FIO_MEMCMP(fio_pub, mul_pub, 65) == 0,
               "fio_p256_keypair should match point_mul");
    FIO_ASSERT(FIO_MEMCMP(mul_pub, openssl_pub, 65) == 0,
               "point_mul should match openssl");
  }

  fprintf(stderr, "P-256 keypair debug stress passed\n");
  return 0;
}
