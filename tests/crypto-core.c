/* *****************************************************************************
Test for 150 crypto core.h

Coverage: the crypto-core module defines the AEAD function-pointer interface
used by TLS and other protocols to abstract cipher choice. This test verifies
the typedefs compile, are usable as function pointers (including the int
success / failure return, with a toy AEAD), that the library AEADs match the
typedefs, and exercises the constant-time conditional-swap idiom documented in
the module header. Cipher correctness belongs to the AES/Chacha tests.
***************************************************************************** */
#include "test-helpers.h"

#define FIO_CRYPTO_CORE
#define FIO_CHACHA
#define FIO_AES
#include FIO_INCLUDE_FILE

/* Toy AEAD obeying the fio_crypto_enc_fn / fio_crypto_dec_fn contract (no
 * real algorithm): returns 0 on success, -1 on failure; a failing call zeroes
 * (enc) or leaves untouched (dec) the caller's buffers. */
#define FIO___CRYPTO_CORE_TEST_MAX 64

static int fio___crypto_core_test_enc(void *restrict mac,
                                      void *restrict data,
                                      size_t len,
                                      const void *ad,
                                      size_t adlen,
                                      const void *key,
                                      const void *nonce) {
  uint8_t *m = (uint8_t *)mac, *d = (uint8_t *)data;
  (void)ad, (void)adlen, (void)nonce;
  if (len > FIO___CRYPTO_CORE_TEST_MAX || !key) {
    FIO_MEMSET(d, 0, len > FIO___CRYPTO_CORE_TEST_MAX ? 0 : len);
    FIO_MEMSET(m, 0, 16);
    return -1;
  }
  uint8_t tag = 0;
  for (size_t i = 0; i < len; ++i)
    tag ^= (d[i] ^= 0x5A);
  FIO_MEMSET(m, tag, 16);
  return 0;
}

static int fio___crypto_core_test_dec(void *restrict mac,
                                      void *restrict data,
                                      size_t len,
                                      const void *ad,
                                      size_t adlen,
                                      const void *key,
                                      const void *nonce) {
  const uint8_t *m = (const uint8_t *)mac;
  uint8_t *d = (uint8_t *)data;
  (void)ad, (void)adlen, (void)nonce;
  if (len > FIO___CRYPTO_CORE_TEST_MAX || !key)
    return -1;
  uint8_t tag = 0;
  for (size_t i = 0; i < len; ++i)
    tag ^= d[i];
  for (size_t i = 0; i < 16; ++i)
    if (m[i] != tag)
      return -1; /* authentication failed: `data` untouched */
  for (size_t i = 0; i < len; ++i)
    d[i] ^= 0x5A;
  return 0;
}

/* The library AEADs must match the typedefs (compile-time signature check). */
static fio_crypto_enc_fn *const fio___crypto_core_enc_impl[] = {
    fio_chacha20_poly1305_enc,
    fio_xchacha20_poly1305_enc,
    fio_aes128_gcm_enc,
    fio_aes256_gcm_enc,
};
static fio_crypto_dec_fn *const fio___crypto_core_dec_impl[] = {
    fio_chacha20_poly1305_dec,
    fio_xchacha20_poly1305_dec,
    fio_aes128_gcm_dec,
    fio_aes256_gcm_dec,
};

int main(void) {
  fprintf(stderr, "* crypto-core tests\n");

  { /* AEAD function-pointer typedefs are usable; int return is honored. */
    fio_crypto_enc_fn *enc = fio___crypto_core_test_enc;
    fio_crypto_dec_fn *dec = fio___crypto_core_test_dec;
    FIO_ASSERT(enc == fio___crypto_core_test_enc,
               "crypto-core enc function pointer mismatch");
    FIO_ASSERT(dec == fio___crypto_core_test_dec,
               "crypto-core dec function pointer mismatch");
    const uint8_t key[32] = {1}, nonce[12] = {2};
    uint8_t mac[16], data[32], orig[32];
    for (size_t i = 0; i < sizeof(data); ++i)
      orig[i] = data[i] = (uint8_t)(i * 7);
    FIO_ASSERT(enc(mac, data, sizeof(data), NULL, 0, key, nonce) == 0,
               "crypto-core enc should return 0 on success");
    FIO_ASSERT(FIO_MEMCMP(data, orig, sizeof(data)),
               "crypto-core enc should transform data");
    /* tampered tag: dec returns -1 and leaves the ciphertext in place */
    uint8_t ct[32];
    FIO_MEMCPY(ct, data, sizeof(ct));
    mac[3] ^= 1;
    FIO_ASSERT(dec(mac, data, sizeof(data), NULL, 0, key, nonce) == -1,
               "crypto-core dec should return -1 on a bad tag");
    FIO_ASSERT(!FIO_MEMCMP(data, ct, sizeof(data)),
               "crypto-core failed dec must not touch data");
    mac[3] ^= 1;
    FIO_ASSERT(dec(mac, data, sizeof(data), NULL, 0, key, nonce) == 0,
               "crypto-core dec should return 0 on success");
    FIO_ASSERT(!FIO_MEMCMP(data, orig, sizeof(data)),
               "crypto-core enc/dec roundtrip failed");
    /* failing enc returns -1 and zeroes the tag */
    FIO_ASSERT(enc(mac, data, sizeof(data), NULL, 0, NULL, nonce) == -1,
               "crypto-core enc should return -1 on failure");
    for (size_t i = 0; i < 16; ++i)
      FIO_ASSERT(!mac[i], "crypto-core failed enc should zero the tag");
  }

  { /* library AEADs through the typedefs: 0 on success, -1 on a bad tag */
    const uint8_t key[32] = {3}, nonce[24] = {4}, ad[5] = {5};
    for (size_t k = 0; k < sizeof(fio___crypto_core_enc_impl) /
                               sizeof(fio___crypto_core_enc_impl[0]);
         ++k) {
      uint8_t mac[16], data[40], orig[40], ct[40];
      for (size_t i = 0; i < sizeof(data); ++i)
        orig[i] = data[i] = (uint8_t)(i + k);
      FIO_ASSERT(fio___crypto_core_enc_impl[k](mac,
                                               data,
                                               sizeof(data),
                                               ad,
                                               sizeof(ad),
                                               key,
                                               nonce) == 0,
                 "crypto-core AEAD %zu enc should return 0",
                 k);
      FIO_MEMCPY(ct, data, sizeof(ct));
      mac[0] ^= 1;
      FIO_ASSERT(fio___crypto_core_dec_impl[k](mac,
                                               data,
                                               sizeof(data),
                                               ad,
                                               sizeof(ad),
                                               key,
                                               nonce) == -1,
                 "crypto-core AEAD %zu dec should reject a bad tag",
                 k);
      /* a failed dec may wipe `data` (unauthenticated plaintext) */
      FIO_MEMCPY(data, ct, sizeof(data));
      mac[0] ^= 1;
      FIO_ASSERT(fio___crypto_core_dec_impl[k](mac,
                                               data,
                                               sizeof(data),
                                               ad,
                                               sizeof(ad),
                                               key,
                                               nonce) == 0,
                 "crypto-core AEAD %zu dec should return 0",
                 k);
      FIO_ASSERT(!FIO_MEMCMP(data, orig, sizeof(data)),
                 "crypto-core AEAD %zu roundtrip failed",
                 k);
    }
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
