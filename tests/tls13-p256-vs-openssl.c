/* *****************************************************************************
Compare fio P-256 keypair/scalar multiplication to OpenSSL.
***************************************************************************** */
#define FIO_LOG
#define FIO_TEST_CSTL
#define FIO_CRYPT
#define FIO_P256
#include "test-helpers.h"

#include <openssl/ec.h>
#include <openssl/bn.h>
#include <openssl/obj_mac.h>

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
  uint8_t secret[32];
  uint8_t fio_pub[65];
  uint8_t ssl_pub[65];

  fio_rand_bytes(secret, 32);
  /* Ensure secret < n by clearing top bits roughly; not strict but fine for
   * comparison. */
  secret[0] &= 0x7F;

  FIO_ASSERT(fio_p256_keypair(secret, fio_pub) == 0,
             "fio_p256_keypair should succeed");
  FIO_ASSERT(p256_public_from_secret_openssl(secret, ssl_pub) == 0,
             "openssl public derivation should succeed");

  if (FIO_MEMCMP(fio_pub, ssl_pub, 65) != 0) {
    fprintf(stderr, "secret: ");
    for (size_t i = 0; i < 32; ++i)
      fprintf(stderr, "%02x", secret[i]);
    fprintf(stderr, "\nfio  pub: ");
    for (size_t i = 0; i < 65; ++i)
      fprintf(stderr, "%02x", fio_pub[i]);
    fprintf(stderr, "\nssl  pub: ");
    for (size_t i = 0; i < 65; ++i)
      fprintf(stderr, "%02x", ssl_pub[i]);
    fprintf(stderr, "\n");
  }
  FIO_ASSERT(FIO_MEMCMP(fio_pub, ssl_pub, 65) == 0,
             "fio P-256 public key should match OpenSSL");
  return 0;
}
