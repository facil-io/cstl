/* *****************************************************************************
RSA-PSS signing speed test. Uses an embedded throwaway 4096-bit RSA key so it
runs in CI/checkouts that don't have the gitignored ./key.pem file.
***************************************************************************** */
#define FIO_LOG
#define FIO_TEST_CSTL
#define FIO_CRYPT
#define FIO_RSA
#define FIO_PEM
#include "tests/test-helpers.h"

/* Throwaway 2048-bit RSA private key in PKCS#8 PEM form (matches tests/pem.c). */
static const char fio___rsa_sign_test_key_pem[] =
    "-----BEGIN PRIVATE KEY-----\n"
    "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\n"
    "0qZTWhC94/EdDbDzbbLibZ08UB0VBte4xw==\n"
    "-----END PRIVATE KEY-----\n";

int main(void) {
  const char *key_pem = fio___rsa_sign_test_key_pem;
  size_t key_pem_len = sizeof(fio___rsa_sign_test_key_pem) - 1;

  fio_pem_private_key_s pkey;
  FIO_ASSERT(fio_pem_parse_private_key(
                 &pkey, key_pem, key_pem_len) == 0,
             "private key parsing should succeed");
  FIO_ASSERT(pkey.type == FIO_PEM_KEY_RSA, "key should be RSA");

  fio_rsa_privkey_s key = {
      .n = pkey.rsa.n,
      .n_len = pkey.rsa.n_len,
      .d = pkey.rsa.d,
      .d_len = pkey.rsa.d_len,
  };

  uint8_t hash[32] = {0};
  uint8_t sig[512];
  size_t sig_len = 0;

  fprintf(stderr, "RSA %zu-bit sign_pss...\n", pkey.rsa.n_len * 8);
  int r = fio_rsa_sign_pss(
      sig, &sig_len, hash, 32, FIO_RSA_HASH_SHA256, &key);
  fprintf(stderr, "result=%d sig_len=%zu\n", r, sig_len);
  FIO_ASSERT(r == 0, "RSA-PSS sign should succeed");

  fio_pem_private_key_clear(&pkey);
  return 0;
}
