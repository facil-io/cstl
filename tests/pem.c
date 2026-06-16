/* *****************************************************************************
Test for 301 pem.h

Coverage: PEM block parsing, X.509 certificate loading from PEM, and private
key loading (PKCS#8, PKCS#1, SEC1). Uses the repo's ./cert.pem and ./key.pem
as real RSA inputs, and uses OpenSSL as a library to cross-check parsed key
material. Also includes edge cases for malformed PEM and unsupported labels.
***************************************************************************** */
#include "test-helpers.h"

#define FIO_TIME
#define FIO_SHA2
#define FIO_MATH
#define FIO_ASN1
#define FIO_RSA
#define FIO_ED25519
#define FIO_P256
#define FIO_STR
#define FIO_X509
#define FIO_PEM
#include FIO_INCLUDE_FILE

#if HAVE_OPENSSL
#include <openssl/bio.h>
#include <openssl/bn.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
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
File loading helper
***************************************************************************** */
FIO_SFUNC char *fio___pem_test_load_file(const char *path, size_t *out_len) {
  FILE *f = fopen(path, "rb");
  FIO_ASSERT(f, "failed to open %s", path);
  FIO_ASSERT(fseek(f, 0, SEEK_END) == 0, "fseek failed");
  long size = ftell(f);
  FIO_ASSERT(size > 0, "empty or missing file %s", path);
  rewind(f);
  char *buf = (char *)FIO_MEM_REALLOC(NULL, 0, (size_t)size + 1, 0);
  FIO_ASSERT(buf, "allocation failed");
  FIO_ASSERT(fread(buf, 1, (size_t)size, f) == (size_t)size,
             "fread failed for %s", path);
  buf[size] = '\0';
  fclose(f);
  *out_len = (size_t)size;
  return buf;
}

/* *****************************************************************************
Core PEM parsing tests
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, pem_parse_block)(void) {
  const char *pem =
      "-----BEGIN CERTIFICATE-----\n"
      "MIIBkTCB+wIJAKHBfpegPjMCMA0GCSqGSIb3DQEBCwUAMBExDzANBgNVBAMMBnVz\n"
      "ZXJzMB4XDTI0MDEwMTAwMDAwMFoXDTI1MDEwMTAwMDAwMFowETEPMA0GA1UEAwwG\n"
      "dXNlcnMwXDANBgkqhkiG9w0BAQEFAANLADBIAkEAx\n"
      "-----END CERTIFICATE-----\n";
  size_t pem_len = FIO_STRLEN(pem);
  uint8_t der[512];
  fio_pem_s block;

  size_t consumed =
      fio_pem_parse(&block, der, sizeof(der), pem, pem_len);
  FIO_ASSERT(consumed > 0, "PEM block parse failed");
  FIO_ASSERT(block.label_len == 11, "label length mismatch");
  FIO_ASSERT(!FIO_MEMCMP(block.label, "CERTIFICATE", 11),
             "label mismatch");
  FIO_ASSERT(block.der == der, "DER buffer pointer mismatch");
  FIO_ASSERT(block.der_len > 0, "DER length should be positive");
}

FIO_SFUNC void FIO_NAME_TEST(stl, pem_parse_certificate)(void) {
  size_t pem_len;
  char *pem = fio___pem_test_load_file("./cert.pem", &pem_len);

  fio_x509_cert_s cert;
  FIO_ASSERT(fio_pem_parse_certificate(&cert, pem, pem_len) == 0,
             "failed to parse ./cert.pem");
  FIO_ASSERT(cert.key_type == FIO_X509_KEY_RSA,
             "./cert.pem should contain an RSA key");
  FIO_ASSERT(cert.pubkey.rsa.n_len >= 256,
             "RSA modulus should be at least 256 bytes");
  FIO_ASSERT(cert.pubkey.rsa.e_len == 3,
             "RSA exponent should be 3 bytes");
  FIO_ASSERT(cert.subject_cn != NULL, "subject CN missing");
  FIO_ASSERT(fio_x509_verify_signature(&cert, &cert) == 0,
             "self-signed ./cert.pem signature should verify");

  FIO_MEM_FREE(pem, pem_len + 1);
}

FIO_SFUNC void FIO_NAME_TEST(stl, pem_get_certificate_der)(void) {
  size_t pem_len;
  char *pem = fio___pem_test_load_file("./cert.pem", &pem_len);

  uint8_t der[4096];
  size_t der_len =
      fio_pem_get_certificate_der(der, sizeof(der), pem, pem_len);
  FIO_ASSERT(der_len > 0, "fio_pem_get_certificate_der failed");

  fio_x509_cert_s cert1, cert2;
  FIO_ASSERT(fio_x509_parse(&cert1, der, der_len) == 0,
             "DER parse failed");
  FIO_ASSERT(fio_pem_parse_certificate(&cert2, pem, pem_len) == 0,
             "PEM parse failed");
  FIO_ASSERT(cert1.pubkey.rsa.n_len == cert2.pubkey.rsa.n_len,
             "DER/PEM parse mismatch");
  FIO_ASSERT(!FIO_MEMCMP(cert1.pubkey.rsa.n, cert2.pubkey.rsa.n,
                         cert1.pubkey.rsa.n_len),
             "modulus mismatch between DER and PEM parse");

  FIO_MEM_FREE(pem, pem_len + 1);
}

FIO_SFUNC void FIO_NAME_TEST(stl, pem_parse_private_key_pkcs8)(void) {
  size_t pem_len;
  char *pem = fio___pem_test_load_file("./key.pem", &pem_len);

  fio_pem_private_key_s key;
  FIO_ASSERT(fio_pem_parse_private_key(&key, pem, pem_len) == 0,
             "failed to parse ./key.pem");
  FIO_ASSERT(key.type == FIO_PEM_KEY_RSA, "./key.pem should be RSA");
  FIO_ASSERT(key.rsa.n_len >= 256, "RSA modulus length mismatch");
  FIO_ASSERT(key.rsa.e_len == 3, "RSA exponent length mismatch");
  FIO_ASSERT(key.rsa.d_len >= 256, "RSA private exponent length mismatch");
  FIO_ASSERT(key.rsa.p_len > 0, "RSA prime p missing");
  FIO_ASSERT(key.rsa.q_len > 0, "RSA prime q missing");
  FIO_ASSERT(key.rsa.dP_len > 0, "RSA dP missing");
  FIO_ASSERT(key.rsa.dQ_len > 0, "RSA dQ missing");
  FIO_ASSERT(key.rsa.qInv_len > 0, "RSA qInv missing");

  fio_pem_private_key_clear(&key);
  FIO_ASSERT(key.type == FIO_PEM_KEY_UNKNOWN,
             "private key clear should reset type");

  FIO_MEM_FREE(pem, pem_len + 1);
}

FIO_SFUNC void FIO_NAME_TEST(stl, pem_invalid_inputs)(void) {
  fio_pem_s block;
  uint8_t der[256];
  FIO_ASSERT(fio_pem_parse(NULL, der, sizeof(der), "x", 1) == 0,
             "NULL out should fail");
  FIO_ASSERT(fio_pem_parse(&block, NULL, sizeof(der), "x", 1) == 0,
             "NULL der buffer should fail");
  FIO_ASSERT(fio_pem_parse(&block, der, sizeof(der), NULL, 1) == 0,
             "NULL PEM data should fail");
  FIO_ASSERT(fio_pem_parse(&block, der, sizeof(der), "not pem", 7) == 0,
             "non-PEM data should fail");

  fio_x509_cert_s cert;
  FIO_ASSERT(fio_pem_parse_certificate(NULL, "x", 1) == -1,
             "NULL cert should fail");
  FIO_ASSERT(fio_pem_parse_certificate(&cert, NULL, 1) == -1,
             "NULL PEM data for certificate should fail");

  fio_pem_private_key_s key;
  FIO_ASSERT(fio_pem_parse_private_key(NULL, "x", 1) == -1,
             "NULL key should fail");
  FIO_ASSERT(fio_pem_parse_private_key(&key, NULL, 1) == -1,
             "NULL PEM data for private key should fail");

  const char *bad_label =
      "-----BEGIN UNKNOWN-----\n"
      "AAAA\n"
      "-----END UNKNOWN-----\n";
  FIO_ASSERT(fio_pem_parse_private_key(&key, bad_label,
                                       FIO_STRLEN(bad_label)) == -1,
             "unsupported private key label should fail");
}

FIO_SFUNC void FIO_NAME_TEST(stl, pem_malformed)(void) {
  const char *missing_end =
      "-----BEGIN CERTIFICATE-----\nAAAA\n";
  uint8_t der[256];
  fio_pem_s block;
  FIO_ASSERT(fio_pem_parse(&block, der, sizeof(der), missing_end,
                           FIO_STRLEN(missing_end)) == 0,
             "missing END marker should fail");

  const char *mismatched_label =
      "-----BEGIN CERTIFICATE-----\n"
      "AAAA\n"
      "-----END PRIVATE KEY-----\n";
  FIO_ASSERT(fio_pem_parse(&block, der, sizeof(der), mismatched_label,
                           FIO_STRLEN(mismatched_label)) == 0,
             "mismatched END label should fail");

  const char *bad_b64 =
      "-----BEGIN CERTIFICATE-----\n"
      "!!!\n"
      "-----END CERTIFICATE-----\n";
  FIO_ASSERT(fio_pem_parse(&block, der, sizeof(der), bad_b64,
                           FIO_STRLEN(bad_b64)) == 0,
             "invalid base64 should fail");
}

/* *****************************************************************************
OpenSSL cross-validation: parsed key material matches OpenSSL's view.
***************************************************************************** */
#if HAVE_OPENSSL

FIO_SFUNC EVP_PKEY *fio___pem_test_load_openssl_key(const char *path) {
  FILE *f = fopen(path, "r");
  FIO_ASSERT(f, "failed to open %s for OpenSSL", path);
  EVP_PKEY *pkey = PEM_read_PrivateKey(f, NULL, NULL, NULL);
  fclose(f);
  FIO_ASSERT(pkey, "OpenSSL failed to load %s", path);
  return pkey;
}

FIO_SFUNC void FIO_NAME_TEST(stl, pem_openssl_key_compare)(void) {
  size_t pem_len;
  char *pem = fio___pem_test_load_file("./key.pem", &pem_len);

  fio_pem_private_key_s fkey;
  FIO_ASSERT(fio_pem_parse_private_key(&fkey, pem, pem_len) == 0,
             "fio failed to parse ./key.pem");

  EVP_PKEY *pkey = fio___pem_test_load_openssl_key("./key.pem");
  RSA *rsa = EVP_PKEY_get1_RSA(pkey);
  FIO_ASSERT(rsa, "EVP_PKEY_get1_RSA failed");

  const BIGNUM *bn_n = NULL, *bn_e = NULL, *bn_d = NULL;
  RSA_get0_key(rsa, &bn_n, &bn_e, &bn_d);
  uint8_t n[FIO_RSA_MAX_BYTES], e[FIO_RSA_MAX_BYTES], d[FIO_RSA_MAX_BYTES];
  size_t n_len = (size_t)BN_num_bytes(bn_n);
  size_t e_len = (size_t)BN_num_bytes(bn_e);
  size_t d_len = (size_t)BN_num_bytes(bn_d);
  BN_bn2bin(bn_n, n);
  BN_bn2bin(bn_e, e);
  BN_bn2bin(bn_d, d);

  FIO_ASSERT(fkey.rsa.n_len == n_len, "modulus length mismatch with OpenSSL");
  FIO_ASSERT(fkey.rsa.e_len == e_len, "exponent length mismatch with OpenSSL");
  FIO_ASSERT(fkey.rsa.d_len == d_len,
             "private exponent length mismatch with OpenSSL");
  FIO_ASSERT(!FIO_MEMCMP(fkey.rsa.n, n, n_len), "modulus mismatch with OpenSSL");
  FIO_ASSERT(!FIO_MEMCMP(fkey.rsa.e, e, e_len), "exponent mismatch with OpenSSL");
  FIO_ASSERT(!FIO_MEMCMP(fkey.rsa.d, d, d_len),
             "private exponent mismatch with OpenSSL");

  RSA_free(rsa);
  EVP_PKEY_free(pkey);
  fio_pem_private_key_clear(&fkey);
  FIO_MEM_FREE(pem, pem_len + 1);
}

FIO_SFUNC void FIO_NAME_TEST(stl, pem_openssl_cert_compare)(void) {
  size_t pem_len;
  char *pem = fio___pem_test_load_file("./cert.pem", &pem_len);

  fio_x509_cert_s fc;
  FIO_ASSERT(fio_pem_parse_certificate(&fc, pem, pem_len) == 0,
             "fio failed to parse ./cert.pem");

  BIO *bio = BIO_new_mem_buf(pem, (int)pem_len);
  FIO_ASSERT(bio, "BIO_new_mem_buf failed");
  X509 *x = PEM_read_bio_X509(bio, NULL, NULL, NULL);
  BIO_free(bio);
  FIO_ASSERT(x, "OpenSSL failed to parse ./cert.pem");

  EVP_PKEY *pkey = X509_get_pubkey(x);
  FIO_ASSERT(pkey, "OpenSSL X509_get_pubkey failed");
  RSA *rsa = EVP_PKEY_get1_RSA(pkey);
  FIO_ASSERT(rsa, "OpenSSL cert public key is not RSA");

  const BIGNUM *bn_n = NULL, *bn_e = NULL;
  RSA_get0_key(rsa, &bn_n, &bn_e, NULL);
  uint8_t n[FIO_RSA_MAX_BYTES], e[FIO_RSA_MAX_BYTES];
  size_t n_len = (size_t)BN_num_bytes(bn_n);
  size_t e_len = (size_t)BN_num_bytes(bn_e);
  BN_bn2bin(bn_n, n);
  BN_bn2bin(bn_e, e);

  FIO_ASSERT(fc.pubkey.rsa.n_len == n_len,
             "cert modulus length mismatch with OpenSSL");
  FIO_ASSERT(fc.pubkey.rsa.e_len == e_len,
             "cert exponent length mismatch with OpenSSL");
  FIO_ASSERT(!FIO_MEMCMP(fc.pubkey.rsa.n, n, n_len),
             "cert modulus mismatch with OpenSSL");
  FIO_ASSERT(!FIO_MEMCMP(fc.pubkey.rsa.e, e, e_len),
             "cert exponent mismatch with OpenSSL");

  RSA_free(rsa);
  EVP_PKEY_free(pkey);
  X509_free(x);
  FIO_MEM_FREE(pem, pem_len + 1);
}

FIO_SFUNC void FIO_NAME_TEST(stl, pem_roundtrip_sign_verify)(void) {
  size_t pem_len;
  char *pem = fio___pem_test_load_file("./key.pem", &pem_len);

  fio_pem_private_key_s fkey;
  FIO_ASSERT(fio_pem_parse_private_key(&fkey, pem, pem_len) == 0,
             "fio failed to parse ./key.pem for roundtrip");

  fio_rsa_privkey_s priv = {.n = fkey.rsa.n,
                            .n_len = fkey.rsa.n_len,
                            .d = fkey.rsa.d,
                            .d_len = fkey.rsa.d_len,
                            .e = fkey.rsa.e,
                            .e_len = fkey.rsa.e_len,
                            .p = fkey.rsa.p,
                            .p_len = fkey.rsa.p_len,
                            .q = fkey.rsa.q,
                            .q_len = fkey.rsa.q_len,
                            .dP = fkey.rsa.dP,
                            .dP_len = fkey.rsa.dP_len,
                            .dQ = fkey.rsa.dQ,
                            .dQ_len = fkey.rsa.dQ_len,
                            .qInv = fkey.rsa.qInv,
                            .qInv_len = fkey.rsa.qInv_len};
  fio_rsa_pubkey_s pub = {.n = fkey.rsa.n,
                          .n_len = fkey.rsa.n_len,
                          .e = fkey.rsa.e,
                          .e_len = fkey.rsa.e_len};

  const uint8_t msg[] = "PEM-loaded RSA key roundtrip";
  fio_u256 h = fio_sha256(msg, sizeof(msg) - 1);
  uint8_t sig[FIO_RSA_MAX_BYTES];
  size_t sig_len = 0;
  FIO_ASSERT(fio_rsa_sign_pss(sig, &sig_len, h.u8, 32,
                              FIO_RSA_HASH_SHA256, &priv) == 0,
             "fio RSA-PSS signing with PEM-loaded key failed");
  FIO_ASSERT(fio_rsa_verify_pss(sig, sig_len, h.u8, 32,
                                FIO_RSA_HASH_SHA256, &pub) == 0,
             "fio RSA-PSS verification with PEM-loaded key failed");

  fio_pem_private_key_clear(&fkey);
  FIO_MEM_FREE(pem, pem_len + 1);
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
  FIO_NAME_TEST(stl, pem_parse_block)();
  FIO_NAME_TEST(stl, pem_parse_certificate)();
  FIO_NAME_TEST(stl, pem_get_certificate_der)();
  FIO_NAME_TEST(stl, pem_parse_private_key_pkcs8)();
  FIO_NAME_TEST(stl, pem_invalid_inputs)();
  FIO_NAME_TEST(stl, pem_malformed)();
#if HAVE_OPENSSL
  FIO_NAME_TEST(stl, pem_openssl_key_compare)();
  FIO_NAME_TEST(stl, pem_openssl_cert_compare)();
  FIO_NAME_TEST(stl, pem_roundtrip_sign_verify)();
#endif
  return 0;
}
