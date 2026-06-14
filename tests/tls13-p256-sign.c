/* *****************************************************************************
P-256 sign/verify sanity test.
***************************************************************************** */
#define FIO_LOG
#define FIO_TEST_CSTL
#define FIO_CRYPT
#define FIO_TLS13
#define FIO_X509
#include "test-helpers.h"

int main(void) {
  fio_x509_keypair_s keypair;
  FIO_ASSERT(fio_x509_keypair_p256(&keypair) == 0,
             "P-256 keypair generation should succeed");

  const uint8_t msg[] = "TLS 1.3, server CertificateVerify";
  fio_u256 hash = fio_sha256(msg, sizeof(msg) - 1);

  uint8_t sig[256];
  size_t sig_len = 0;
  FIO_ASSERT(fio_ecdsa_p256_sign(
                 sig, &sig_len, sizeof(sig), hash.u8, keypair.secret_key) == 0,
             "P-256 sign should succeed");

  FIO_ASSERT(fio_ecdsa_p256_verify(
                 sig, sig_len, hash.u8, keypair.public_key, 65) == 0,
             "P-256 verify should succeed");

  fio_x509_keypair_clear(&keypair);
  return 0;
}
