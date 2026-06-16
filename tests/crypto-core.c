/* *****************************************************************************
Test for 150 crypto core.h

Coverage: the crypto-core module defines the AEAD function-pointer interface
used by TLS and other protocols to abstract cipher choice. This test verifies
the typedefs compile, are usable as function pointers, and exercise the
constant-time conditional-swap idiom documented in the module header. It does
not implement or test any specific cipher (those belong to SHA/AES/Chacha/etc.
tests).
***************************************************************************** */
#include "test-helpers.h"

#define FIO_CRYPTO_CORE
#include FIO_INCLUDE_FILE

/* Dummy encoder matching fio_crypto_enc_fn. No real algorithm. */
static void fio___crypto_core_test_enc(void *restrict mac,
                                       void *restrict data,
                                       size_t len,
                                       const void *ad,
                                       size_t adlen,
                                       const void *key,
                                       const void *nonce) {
  (void)mac, (void)data, (void)len;
  (void)ad, (void)adlen;
  (void)key, (void)nonce;
}

/* Dummy decoder matching fio_crypto_dec_fn. No real algorithm. */
static int fio___crypto_core_test_dec(void *restrict mac,
                                      void *restrict data,
                                      size_t len,
                                      const void *ad,
                                      size_t adlen,
                                      const void *key,
                                      const void *nonce) {
  (void)mac, (void)data, (void)len;
  (void)ad, (void)adlen;
  (void)key, (void)nonce;
  return 0;
}

int main(void) {
  fprintf(stderr, "* crypto-core tests\n");

  { /* AEAD function-pointer typedefs are usable. */
    fio_crypto_enc_fn *enc = fio___crypto_core_test_enc;
    fio_crypto_dec_fn *dec = fio___crypto_core_test_dec;
    FIO_ASSERT(enc == fio___crypto_core_test_enc,
               "crypto-core enc function pointer mismatch");
    FIO_ASSERT(dec == fio___crypto_core_test_dec,
               "crypto-core dec function pointer mismatch");
    FIO_ASSERT(enc != NULL, "crypto-core enc function pointer is NULL");
    FIO_ASSERT(dec != NULL, "crypto-core dec function pointer is NULL");
  }

  { /* Constant-time conditional swap idiom from crypto-core docs. */
    uint64_t a = 0xAAAAAAAAAAAAAAAAULL;
    uint64_t b = 0xBBBBBBBBBBBBBBBBULL;
    uint64_t condition = 1;
    uint64_t mask = 0 - condition;
    uint64_t t = mask & (a ^ b);
    a ^= t;
    b ^= t;
    FIO_ASSERT(a == 0xBBBBBBBBBBBBBBBBULL && b == 0xAAAAAAAAAAAAAAAAULL,
               "crypto-core cswap failed when condition is true");

    a = 0xAAAAAAAAAAAAAAAAULL;
    b = 0xBBBBBBBBBBBBBBBBULL;
    condition = 0;
    mask = 0 - condition;
    t = mask & (a ^ b);
    a ^= t;
    b ^= t;
    FIO_ASSERT(a == 0xAAAAAAAAAAAAAAAAULL && b == 0xBBBBBBBBBBBBBBBBULL,
               "crypto-core cswap failed when condition is false");
  }

  fprintf(stderr, "* crypto-core tests passed\n");
  return 0;
}
