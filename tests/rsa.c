/* *****************************************************************************
Test for 155 rsa.h

Coverage: RSA PKCS#1 v1.5 and RSA-PSS signature verification and signing.
Uses OpenSSL as a library to generate a real 2048-bit RSA keypair and to
verify signatures produced by fio, and vice versa. Also includes edge-case
and invalid-input assertions ported from the archived tests.
***************************************************************************** */
#include "test-helpers.h"

#define FIO_MATH
#define FIO_SHA2
#define FIO_RAND
#define FIO_RSA
#include FIO_INCLUDE_FILE

#if HAVE_OPENSSL
#include <openssl/bn.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
#endif

/* *****************************************************************************
Hex conversion helper
***************************************************************************** */
FIO_SFUNC size_t fio___rsa_test_hex2bin(uint8_t *out,
                                        size_t out_len,
                                        const char *hex) {
  size_t len = 0;
  while (*hex && len < out_len) {
    int hi = 0, lo = 0;
    if (*hex >= '0' && *hex <= '9')
      hi = *hex - '0';
    else if (*hex >= 'a' && *hex <= 'f')
      hi = *hex - 'a' + 10;
    else if (*hex >= 'A' && *hex <= 'F')
      hi = *hex - 'A' + 10;
    else {
      ++hex;
      continue;
    }
    ++hex;
    if (!*hex)
      break;
    if (*hex >= '0' && *hex <= '9')
      lo = *hex - '0';
    else if (*hex >= 'a' && *hex <= 'f')
      lo = *hex - 'a' + 10;
    else if (*hex >= 'A' && *hex <= 'F')
      lo = *hex - 'A' + 10;
    ++hex;
    out[len++] = (uint8_t)((hi << 4) | lo);
  }
  return len;
}

/* *****************************************************************************
Byte/word conversion helpers (archived coverage)
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, rsa_byte_conversion)(void) {
  uint8_t bytes[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                       0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};
  uint64_t words[2];

  fio___rsa_bytes_to_words(words, 2, bytes, 16);
  FIO_ASSERT(words[0] == 0x090A0B0C0D0E0F10ULL,
             "RSA LSW conversion failed");
  FIO_ASSERT(words[1] == 0x0102030405060708ULL,
             "RSA MSW conversion failed");

  uint8_t bytes2[16];
  fio___rsa_words_to_bytes(bytes2, 16, words, 2);
  FIO_ASSERT(!FIO_MEMCMP(bytes, bytes2, 16), "RSA round-trip conversion failed");
}

/* *****************************************************************************
Static RSA key for tests that must not depend on OpenSSL runtime generation.
This is the well-known 2048-bit test modulus used in the archived rsa.c tests.
It is NOT a valid keypair for signing, so it is used only for parse/operation
and invalid-signature checks.
***************************************************************************** */
static const char *fio___rsa_test_n_hex =
    "b3510a2bcd4ce644c5b594ae5059e12b2f054b658d5da5959a2fdf1871b808bc"
    "3df3e628d2792e51aad5c124b43bda453cd95f3de3c4b5bfb26c5c9547c9b0c1"
    "b3cd6f8a9c7c1f1b9c9f8b3a9f8d5a7e9b3c5a7e8b2c4a6e8b2c4a6e8b2c4a6e"
    "8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e"
    "8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e"
    "8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e"
    "8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e"
    "8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e"
    "8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a6e8b2c4a01";
static const char *fio___rsa_test_e_hex = "010001";

FIO_SFUNC void FIO_NAME_TEST(stl, rsa_public_op)(void) {
  uint8_t n[256], e[4], sig[256], result[256];
  size_t n_len = fio___rsa_test_hex2bin(n, sizeof(n), fio___rsa_test_n_hex);
  size_t e_len = fio___rsa_test_hex2bin(e, sizeof(e), fio___rsa_test_e_hex);
  FIO_ASSERT(n_len == 256, "test modulus length mismatch");
  FIO_ASSERT(e_len == 3, "test exponent length mismatch");

  FIO_MEMSET(sig, 0, sizeof(sig));
  sig[1] = 0x01;
  for (size_t i = 2; i < 256; ++i)
    sig[i] = (uint8_t)(i & 0xFF);

  fio_rsa_pubkey_s key = {.n = n, .n_len = n_len, .e = e, .e_len = e_len};
  FIO_ASSERT(fio___rsa_public_op(result, sig, 256, &key) == 0,
             "RSA public op failed");

  int all_zero = 1;
  for (size_t i = 0; i < 256; ++i) {
    if (result[i] != 0) {
      all_zero = 0;
      break;
    }
  }
  FIO_ASSERT(!all_zero, "RSA public op returned all zeros");
}

FIO_SFUNC void FIO_NAME_TEST(stl, rsa_mgf1)(void) {
  uint8_t seed[32];
  FIO_MEMSET(seed, 0x42, 32);
  uint8_t mask[64], mask2[64];

  fio___rsa_mgf1(mask, 64, seed, 32, FIO_RSA_HASH_SHA256);
  int all_zero = 1;
  for (size_t i = 0; i < 64; ++i) {
    if (mask[i] != 0) {
      all_zero = 0;
      break;
    }
  }
  FIO_ASSERT(!all_zero, "MGF1 mask should not be all zeros");

  fio___rsa_mgf1(mask2, 64, seed, 32, FIO_RSA_HASH_SHA256);
  FIO_ASSERT(!FIO_MEMCMP(mask, mask2, 64), "MGF1 should be deterministic");

  uint8_t seed2[32];
  FIO_MEMSET(seed2, 0x43, 32);
  fio___rsa_mgf1(mask2, 64, seed2, 32, FIO_RSA_HASH_SHA256);
  FIO_ASSERT(FIO_MEMCMP(mask, mask2, 64) != 0,
             "Different seeds should produce different MGF1 masks");
}

FIO_SFUNC void FIO_NAME_TEST(stl, rsa_invalid_inputs)(void) {
  uint8_t sig[256], hash[32], n[256], e[3];
  FIO_MEMSET(sig, 0, sizeof(sig));
  FIO_MEMSET(hash, 0xAB, sizeof(hash));
  fio___rsa_test_hex2bin(n, sizeof(n), fio___rsa_test_n_hex);
  fio___rsa_test_hex2bin(e, sizeof(e), fio___rsa_test_e_hex);

  fio_rsa_pubkey_s key = {.n = n, .n_len = 256, .e = e, .e_len = 3};

  FIO_ASSERT(fio_rsa_verify_pkcs1(NULL, 256, hash, 32,
                                  FIO_RSA_HASH_SHA256, &key) == -1,
             "NULL sig should fail PKCS#1");
  FIO_ASSERT(fio_rsa_verify_pkcs1(sig, 256, NULL, 32,
                                  FIO_RSA_HASH_SHA256, &key) == -1,
             "NULL hash should fail PKCS#1");
  FIO_ASSERT(fio_rsa_verify_pkcs1(sig, 256, hash, 32,
                                  FIO_RSA_HASH_SHA256, NULL) == -1,
             "NULL key should fail PKCS#1");
  FIO_ASSERT(fio_rsa_verify_pkcs1(sig, 256, hash, 16,
                                  FIO_RSA_HASH_SHA256, &key) == -1,
             "bad hash length should fail PKCS#1");
  FIO_ASSERT(fio_rsa_verify_pkcs1(sig, 128, hash, 32,
                                  FIO_RSA_HASH_SHA256, &key) == -1,
             "sig length mismatch should fail PKCS#1");

  FIO_ASSERT(fio_rsa_verify_pss(NULL, 256, hash, 32,
                                FIO_RSA_HASH_SHA256, &key) == -1,
             "NULL sig should fail PSS");
  FIO_ASSERT(fio_rsa_verify_pss(sig, 256, NULL, 32,
                                FIO_RSA_HASH_SHA256, &key) == -1,
             "NULL hash should fail PSS");
  FIO_ASSERT(fio_rsa_verify_pss(sig, 256, hash, 32,
                                FIO_RSA_HASH_SHA256, NULL) == -1,
             "NULL key should fail PSS");

  FIO_MEMSET(sig, 0x42, sizeof(sig));
  FIO_ASSERT(fio_rsa_verify_pkcs1(sig, 256, hash, 32,
                                  FIO_RSA_HASH_SHA256, &key) == -1,
             "random signature should fail PKCS#1");
  FIO_ASSERT(fio_rsa_verify_pss(sig, 256, hash, 32,
                                FIO_RSA_HASH_SHA256, &key) == -1,
             "random signature should fail PSS");

  fio_rsa_pubkey_s bad_key1 = {.n = n, .n_len = 0, .e = e, .e_len = 3};
  FIO_ASSERT(fio_rsa_verify_pkcs1(sig, 256, hash, 32,
                                  FIO_RSA_HASH_SHA256, &bad_key1) == -1,
             "empty modulus should fail");
  fio_rsa_pubkey_s bad_key2 = {.n = n, .n_len = 256, .e = e, .e_len = 0};
  FIO_ASSERT(fio_rsa_verify_pkcs1(sig, 256, hash, 32,
                                  FIO_RSA_HASH_SHA256, &bad_key2) == -1,
             "empty exponent should fail");
}

/* *****************************************************************************
OpenSSL cross-validation tests
***************************************************************************** */
#if HAVE_OPENSSL

FIO_SFUNC EVP_PKEY *fio___rsa_test_gen_key(void) {
  EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
  FIO_ASSERT(ctx, "EVP_PKEY_CTX_new_id failed");
  FIO_ASSERT(EVP_PKEY_keygen_init(ctx) == 1, "EVP_PKEY_keygen_init failed");
  FIO_ASSERT(EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, 2048) == 1,
             "EVP_PKEY_CTX_set_rsa_keygen_bits failed");
  EVP_PKEY *pkey = NULL;
  FIO_ASSERT(EVP_PKEY_keygen(ctx, &pkey) == 1, "EVP_PKEY_keygen failed");
  EVP_PKEY_CTX_free(ctx);
  return pkey;
}

FIO_SFUNC void fio___rsa_test_extract_key(EVP_PKEY *pkey,
                                          uint8_t *n,
                                          size_t *n_len,
                                          uint8_t *e,
                                          size_t *e_len,
                                          uint8_t *d,
                                          size_t *d_len,
                                          uint8_t *p,
                                          size_t *p_len,
                                          uint8_t *q,
                                          size_t *q_len,
                                          uint8_t *dP,
                                          size_t *dP_len,
                                          uint8_t *dQ,
                                          size_t *dQ_len,
                                          uint8_t *qInv,
                                          size_t *qInv_len) {
  RSA *rsa = EVP_PKEY_get1_RSA(pkey);
  FIO_ASSERT(rsa, "EVP_PKEY_get1_RSA failed");
  const BIGNUM *bn_n = NULL, *bn_e = NULL, *bn_d = NULL;
  const BIGNUM *bn_p = NULL, *bn_q = NULL, *bn_dP = NULL, *bn_dQ = NULL,
               *bn_qInv = NULL;
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
  RSA_get0_key(rsa, &bn_n, &bn_e, &bn_d);
  RSA_get0_factors(rsa, &bn_p, &bn_q);
  RSA_get0_crt_params(rsa, &bn_dP, &bn_dQ, &bn_qInv);
#else
  bn_n = rsa->n;
  bn_e = rsa->e;
  bn_d = rsa->d;
  bn_p = rsa->p;
  bn_q = rsa->q;
  bn_dP = rsa->dmp1;
  bn_dQ = rsa->dmq1;
  bn_qInv = rsa->iqmp;
#endif

  *n_len = (size_t)BN_num_bytes(bn_n);
  *e_len = (size_t)BN_num_bytes(bn_e);
  *d_len = (size_t)BN_num_bytes(bn_d);
  FIO_ASSERT(*n_len <= FIO_RSA_MAX_BYTES, "modulus too large");
  FIO_ASSERT(*e_len <= FIO_RSA_MAX_BYTES, "exponent too large");
  FIO_ASSERT(*d_len <= FIO_RSA_MAX_BYTES, "private exponent too large");
  BN_bn2bin(bn_n, n);
  BN_bn2bin(bn_e, e);
  BN_bn2bin(bn_d, d);

  if (bn_p) {
    *p_len = (size_t)BN_num_bytes(bn_p);
    BN_bn2bin(bn_p, p);
  } else
    *p_len = 0;
  if (bn_q) {
    *q_len = (size_t)BN_num_bytes(bn_q);
    BN_bn2bin(bn_q, q);
  } else
    *q_len = 0;
  if (bn_dP) {
    *dP_len = (size_t)BN_num_bytes(bn_dP);
    BN_bn2bin(bn_dP, dP);
  } else
    *dP_len = 0;
  if (bn_dQ) {
    *dQ_len = (size_t)BN_num_bytes(bn_dQ);
    BN_bn2bin(bn_dQ, dQ);
  } else
    *dQ_len = 0;
  if (bn_qInv) {
    *qInv_len = (size_t)BN_num_bytes(bn_qInv);
    BN_bn2bin(bn_qInv, qInv);
  } else
    *qInv_len = 0;

  RSA_free(rsa);
}

FIO_SFUNC void FIO_NAME_TEST(stl, rsa_pss_fio_sign_openssl_verify)(void) {
  EVP_PKEY *pkey = fio___rsa_test_gen_key();

  uint8_t n[FIO_RSA_MAX_BYTES], e[FIO_RSA_MAX_BYTES], d[FIO_RSA_MAX_BYTES];
  uint8_t p[FIO_RSA_MAX_BYTES], q[FIO_RSA_MAX_BYTES];
  uint8_t dP[FIO_RSA_MAX_BYTES], dQ[FIO_RSA_MAX_BYTES], qInv[FIO_RSA_MAX_BYTES];
  size_t n_len, e_len, d_len, p_len, q_len, dP_len, dQ_len, qInv_len;

  fio___rsa_test_extract_key(pkey, n, &n_len, e, &e_len, d, &d_len,
                             p, &p_len, q, &q_len, dP, &dP_len,
                             dQ, &dQ_len, qInv, &qInv_len);

  fio_rsa_privkey_s priv = {.n = n,
                            .n_len = n_len,
                            .d = d,
                            .d_len = d_len,
                            .e = e,
                            .e_len = e_len,
                            .p = p,
                            .p_len = p_len,
                            .q = q,
                            .q_len = q_len,
                            .dP = dP,
                            .dP_len = dP_len,
                            .dQ = dQ,
                            .dQ_len = dQ_len,
                            .qInv = qInv,
                            .qInv_len = qInv_len};
  fio_rsa_pubkey_s pub = {.n = n, .n_len = n_len, .e = e, .e_len = e_len};

  const uint8_t msg[] = "RSA-PSS cross-validation message";
  fio_u256 h = fio_sha256(msg, sizeof(msg) - 1);

  uint8_t sig[FIO_RSA_MAX_BYTES];
  size_t sig_len = 0;
  FIO_ASSERT(fio_rsa_sign_pss(sig, &sig_len, h.u8, 32,
                              FIO_RSA_HASH_SHA256, &priv) == 0,
             "fio RSA-PSS signing failed");
  FIO_ASSERT(sig_len == n_len, "signature length mismatch");
  FIO_ASSERT(fio_rsa_verify_pss(sig, sig_len, h.u8, 32,
                                FIO_RSA_HASH_SHA256, &pub) == 0,
             "fio RSA-PSS self-verification failed");

  EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
  FIO_ASSERT(mdctx, "EVP_MD_CTX_new failed");
  FIO_ASSERT(EVP_DigestVerifyInit(mdctx, NULL, EVP_sha256(), NULL, pkey) == 1,
             "EVP_DigestVerifyInit failed");
  FIO_ASSERT(EVP_PKEY_CTX_set_rsa_padding(
                 EVP_MD_CTX_get_pkey_ctx(mdctx), RSA_PKCS1_PSS_PADDING) == 1,
             "EVP_PKEY_CTX_set_rsa_padding failed");
  FIO_ASSERT(EVP_PKEY_CTX_set_rsa_pss_saltlen(
                 EVP_MD_CTX_get_pkey_ctx(mdctx), 32) == 1,
             "EVP_PKEY_CTX_set_rsa_pss_saltlen failed");
  int rc = EVP_DigestVerify(mdctx, sig, sig_len, msg, sizeof(msg) - 1);
  FIO_ASSERT(rc == 1, "OpenSSL should verify fio RSA-PSS signature");
  EVP_MD_CTX_free(mdctx);

  EVP_PKEY_free(pkey);
}

FIO_SFUNC void FIO_NAME_TEST(stl, rsa_pss_openssl_sign_fio_verify)(void) {
  EVP_PKEY *pkey = fio___rsa_test_gen_key();

  uint8_t n[FIO_RSA_MAX_BYTES], e[FIO_RSA_MAX_BYTES], d[FIO_RSA_MAX_BYTES];
  uint8_t p[FIO_RSA_MAX_BYTES], q[FIO_RSA_MAX_BYTES];
  uint8_t dP[FIO_RSA_MAX_BYTES], dQ[FIO_RSA_MAX_BYTES], qInv[FIO_RSA_MAX_BYTES];
  size_t n_len, e_len, d_len, p_len, q_len, dP_len, dQ_len, qInv_len;

  fio___rsa_test_extract_key(pkey, n, &n_len, e, &e_len, d, &d_len,
                             p, &p_len, q, &q_len, dP, &dP_len,
                             dQ, &dQ_len, qInv, &qInv_len);

  fio_rsa_pubkey_s pub = {.n = n, .n_len = n_len, .e = e, .e_len = e_len};

  const uint8_t msg[] = "OpenSSL RSA-PSS signed message";
  fio_u256 h = fio_sha256(msg, sizeof(msg) - 1);

  EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
  FIO_ASSERT(mdctx, "EVP_MD_CTX_new failed");
  FIO_ASSERT(EVP_DigestSignInit(mdctx, NULL, EVP_sha256(), NULL, pkey) == 1,
             "EVP_DigestSignInit failed");
  FIO_ASSERT(EVP_PKEY_CTX_set_rsa_padding(
                 EVP_MD_CTX_get_pkey_ctx(mdctx), RSA_PKCS1_PSS_PADDING) == 1,
             "EVP_PKEY_CTX_set_rsa_padding failed");
  FIO_ASSERT(EVP_PKEY_CTX_set_rsa_pss_saltlen(
                 EVP_MD_CTX_get_pkey_ctx(mdctx), 32) == 1,
             "EVP_PKEY_CTX_set_rsa_pss_saltlen failed");
  size_t sig_len = 0;
  FIO_ASSERT(EVP_DigestSign(mdctx, NULL, &sig_len, msg, sizeof(msg) - 1) == 1,
             "EVP_DigestSign size query failed");
  FIO_ASSERT(sig_len == n_len, "OpenSSL signature length mismatch");
  uint8_t sig[FIO_RSA_MAX_BYTES];
  FIO_ASSERT(EVP_DigestSign(mdctx, sig, &sig_len, msg, sizeof(msg) - 1) == 1,
             "OpenSSL RSA-PSS signing failed");
  EVP_MD_CTX_free(mdctx);

  FIO_ASSERT(fio_rsa_verify_pss(sig, sig_len, h.u8, 32,
                                FIO_RSA_HASH_SHA256, &pub) == 0,
             "fio should verify OpenSSL RSA-PSS signature");

  EVP_PKEY_free(pkey);
}

FIO_SFUNC void FIO_NAME_TEST(stl, rsa_pkcs1_openssl_sign_fio_verify)(void) {
  EVP_PKEY *pkey = fio___rsa_test_gen_key();

  uint8_t n[FIO_RSA_MAX_BYTES], e[FIO_RSA_MAX_BYTES], d[FIO_RSA_MAX_BYTES];
  size_t n_len, e_len, d_len;
  {
    uint8_t p[FIO_RSA_MAX_BYTES], q[FIO_RSA_MAX_BYTES];
    uint8_t dP[FIO_RSA_MAX_BYTES], dQ[FIO_RSA_MAX_BYTES],
        qInv[FIO_RSA_MAX_BYTES];
    size_t p_len, q_len, dP_len, dQ_len, qInv_len;
    fio___rsa_test_extract_key(pkey, n, &n_len, e, &e_len, d, &d_len,
                               p, &p_len, q, &q_len, dP, &dP_len,
                               dQ, &dQ_len, qInv, &qInv_len);
  }

  fio_rsa_pubkey_s pub = {.n = n, .n_len = n_len, .e = e, .e_len = e_len};

  const uint8_t msg[] = "OpenSSL PKCS#1 v1.5 signed message";
  fio_u256 h = fio_sha256(msg, sizeof(msg) - 1);

  EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
  FIO_ASSERT(mdctx, "EVP_MD_CTX_new failed");
  FIO_ASSERT(EVP_DigestSignInit(mdctx, NULL, EVP_sha256(), NULL, pkey) == 1,
             "EVP_DigestSignInit failed");
  size_t sig_len = 0;
  FIO_ASSERT(EVP_DigestSign(mdctx, NULL, &sig_len, msg, sizeof(msg) - 1) == 1,
             "EVP_DigestSign size query failed");
  FIO_ASSERT(sig_len == n_len, "OpenSSL PKCS#1 signature length mismatch");
  uint8_t sig[FIO_RSA_MAX_BYTES];
  FIO_ASSERT(EVP_DigestSign(mdctx, sig, &sig_len, msg, sizeof(msg) - 1) == 1,
             "OpenSSL PKCS#1 signing failed");
  EVP_MD_CTX_free(mdctx);

  FIO_ASSERT(fio_rsa_verify_pkcs1(sig, sig_len, h.u8, 32,
                                  FIO_RSA_HASH_SHA256, &pub) == 0,
             "fio should verify OpenSSL PKCS#1 v1.5 signature");

  EVP_PKEY_free(pkey);
}

FIO_SFUNC void FIO_NAME_TEST(stl, rsa_pkcs1_fio_rejects_bad_sig)(void) {
  EVP_PKEY *pkey = fio___rsa_test_gen_key();

  uint8_t n[FIO_RSA_MAX_BYTES], e[FIO_RSA_MAX_BYTES], d[FIO_RSA_MAX_BYTES];
  size_t n_len, e_len, d_len;
  {
    uint8_t p[FIO_RSA_MAX_BYTES], q[FIO_RSA_MAX_BYTES];
    uint8_t dP[FIO_RSA_MAX_BYTES], dQ[FIO_RSA_MAX_BYTES],
        qInv[FIO_RSA_MAX_BYTES];
    size_t p_len, q_len, dP_len, dQ_len, qInv_len;
    fio___rsa_test_extract_key(pkey, n, &n_len, e, &e_len, d, &d_len,
                               p, &p_len, q, &q_len, dP, &dP_len,
                               dQ, &dQ_len, qInv, &qInv_len);
  }

  fio_rsa_pubkey_s pub = {.n = n, .n_len = n_len, .e = e, .e_len = e_len};

  const uint8_t msg[] = "Message with corrupted signature";
  fio_u256 h = fio_sha256(msg, sizeof(msg) - 1);

  EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
  FIO_ASSERT(mdctx, "EVP_MD_CTX_new failed");
  FIO_ASSERT(EVP_DigestSignInit(mdctx, NULL, EVP_sha256(), NULL, pkey) == 1,
             "EVP_DigestSignInit failed");
  size_t sig_len = 0;
  FIO_ASSERT(EVP_DigestSign(mdctx, NULL, &sig_len, msg, sizeof(msg) - 1) == 1,
             "EVP_DigestSign size query failed");
  uint8_t sig[FIO_RSA_MAX_BYTES];
  FIO_ASSERT(EVP_DigestSign(mdctx, sig, &sig_len, msg, sizeof(msg) - 1) == 1,
             "OpenSSL signing failed");
  EVP_MD_CTX_free(mdctx);

  sig[sig_len - 1] ^= 0xFF;
  FIO_ASSERT(fio_rsa_verify_pkcs1(sig, sig_len, h.u8, 32,
                                  FIO_RSA_HASH_SHA256, &pub) == -1,
             "fio should reject corrupted PKCS#1 signature");

  EVP_PKEY_free(pkey);
}

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#endif /* HAVE_OPENSSL */

/* *****************************************************************************
Main
***************************************************************************** */
int main(void) {
  FIO_NAME_TEST(stl, rsa_byte_conversion)();
  FIO_NAME_TEST(stl, rsa_public_op)();
  FIO_NAME_TEST(stl, rsa_mgf1)();
  FIO_NAME_TEST(stl, rsa_invalid_inputs)();
#if HAVE_OPENSSL
  FIO_NAME_TEST(stl, rsa_pss_fio_sign_openssl_verify)();
  FIO_NAME_TEST(stl, rsa_pss_openssl_sign_fio_verify)();
  FIO_NAME_TEST(stl, rsa_pkcs1_openssl_sign_fio_verify)();
  FIO_NAME_TEST(stl, rsa_pkcs1_fio_rejects_bad_sig)();
#endif
  return 0;
}
