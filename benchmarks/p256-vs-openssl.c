/* *****************************************************************************
P-256 comparison benchmark against OpenSSL (154 p256.h)

Merges the old tls13-p256-vs-openssl.c and tls13-p256-ops-vs-openssl.c
comparisons into a single library-level benchmark.  No external processes are
spawned; OpenSSL is used through libcrypto only.

The benchmark repeatedly:
  1. Derives a P-256 public key from a random secret with fio_p256_keypair and
     compares it to OpenSSL's EC_POINT_mul-by-generator result.
  2. Performs scalar-by-base-point multiplication with fio___p256_point_mul for
     k=1 and k=2 and compares the results to OpenSSL's EC_POINT_mul.
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
 * that exposes arbitrary scalar-by-base-point multiplication, so this benchmark
 * keeps using EC_KEY locally and silences the warning. */
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

static const uint8_t P256_G[65] = {
    0x04, 0x6b, 0x17, 0xd1, 0xf2, 0xe1, 0x2c, 0x42, 0x47, 0xf8, 0xbc, 0xe6,
    0xe5, 0x63, 0xa4, 0x40, 0xf2, 0x77, 0x03, 0x7d, 0x81, 0x2d, 0xeb, 0x33,
    0xa0, 0xf4, 0xa1, 0x39, 0x45, 0xd8, 0x98, 0xc2, 0x96, 0x4f, 0xe3, 0x42,
    0xe2, 0xfe, 0x1a, 0x7f, 0x9b, 0x8e, 0xe7, 0xeb, 0x4a, 0x7c, 0x0f, 0x9e,
    0x16, 0x2b, 0xce, 0x33, 0x57, 0x6b, 0x31, 0x5e, 0xce, 0xcb, 0xb6, 0x40,
    0x68, 0x37, 0xbf, 0x51, 0xf5};

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

static void p256_dump(const char *label, const uint8_t *buf, size_t len) {
  fprintf(stderr, "%s", label);
  for (size_t i = 0; i < len; ++i)
    fprintf(stderr, "%02x", buf[i]);
  fprintf(stderr, "\n");
}

int main(void) {
  const size_t rounds = 64;
  uint8_t secret[32];
  uint8_t fio_pub[65];
  uint8_t ssl_pub[65];

  fprintf(stderr, "P-256 keypair vs OpenSSL (%zu random secrets)...\n", rounds);
  int64_t start = FIO_NAME_TEST(stl, atol_time)();
  for (size_t r = 0; r < rounds; ++r) {
    fio_rand_bytes(secret, 32);
    secret[0] &= 0x7F;

    FIO_ASSERT(fio_p256_keypair(secret, fio_pub) == 0,
               "fio_p256_keypair should succeed");
    FIO_ASSERT(p256_public_from_secret_openssl(secret, ssl_pub) == 0,
               "openssl public derivation should succeed");
    if (FIO_MEMCMP(fio_pub, ssl_pub, 65) != 0) {
      p256_dump("secret: ", secret, 32);
      p256_dump("fio:  ", fio_pub, 65);
      p256_dump("ssl:  ", ssl_pub, 65);
    }
    FIO_ASSERT(FIO_MEMCMP(fio_pub, ssl_pub, 65) == 0,
               "fio P-256 public key should match OpenSSL");
  }
  fprintf(stderr,
          "  keypair comparison: %zu rounds in %lld us\n",
          rounds,
          (long long)(FIO_NAME_TEST(stl, atol_time)() - start));

  fprintf(stderr, "P-256 point_mul vs OpenSSL (k=1, k=2)...\n");
  fio___p256_point_affine_s g;
  fio___p256_fe_from_bytes(g.x, P256_G + 1);
  fio___p256_fe_from_bytes(g.y, P256_G + 33);

  for (uint8_t k_val = 1; k_val <= 2; ++k_val) {
    uint8_t k[32] = {0};
    k[31] = k_val;
    fio___p256_scalar_s s;
    fio___p256_scalar_from_bytes(s, k);
    fio___p256_point_jacobian_s R;
    fio___p256_point_mul(&R, s, &g);
    fio___p256_point_affine_s A;
    fio___p256_point_to_affine(&A, &R);
    uint8_t fio_out[65];
    fio_out[0] = 0x04;
    fio___p256_fe_to_bytes(fio_out + 1, A.x);
    fio___p256_fe_to_bytes(fio_out + 33, A.y);

    FIO_ASSERT(p256_mul_openssl(k, ssl_pub) == 0,
               "openssl k=%d mul should succeed", (int)k_val);
    if (FIO_MEMCMP(fio_out, ssl_pub, 65) != 0) {
      fprintf(stderr, "k=%d mismatch\n", (int)k_val);
      p256_dump("fio: ", fio_out, 65);
      p256_dump("ssl: ", ssl_pub, 65);
    }
    FIO_ASSERT(FIO_MEMCMP(fio_out, ssl_pub, 65) == 0,
               "fio k=%d point mul should match OpenSSL", (int)k_val);
  }

  fprintf(stderr, "P-256 vs OpenSSL benchmark passed\n");
  return 0;
}
