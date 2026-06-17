/* *****************************************************************************
Test for 301 pem.h

Coverage: PEM block parsing, X.509 certificate loading from PEM, and private
key loading (PKCS#8, PKCS#1, SEC1). Uses embedded RSA certificate and
private-key PEM fixtures as inputs, and uses OpenSSL as a library to
cross-check parsed key material. Also includes edge cases for malformed PEM
and unsupported labels.
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
Embedded PEM test fixtures (throwaway 4096-bit RSA key / self-signed cert)
***************************************************************************** */
static const char fio___pem_test_cert_pem[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIEpDCCAowCCQD7nLVx5fjsCTANBgkqhkiG9w0BAQsFADAUMRIwEAYDVQQDDAls\n"
    "b2FjbGhvc3QwHhcNMjMwMjI1MTc1NzMxWhcNMzMwMjIyMTc1NzMxWjAUMRIwEAYD\n"
    "VQQDDAlsb2FjbGhvc3QwggIiMA0GCSqGSIb3DQEBAQUAA4ICDwAwggIKAoICAQDx\n"
    "UxjNpGW+GUt5i+OEQ9sSlY3XZT9PquA9GUCjmgye2Vu3o7c58uhyCuMki8JjMVK2\n"
    "hs4cwkgSa/XnkdLuwLRZjMKdelBt35SvP9dA+N91FwAvWNF0FryS7BZ10xDymrD7\n"
    "qBCTCxVFQfWsz+2WS0o2Zck6C8mAa6PUBH2QFZP9rzff5cjkiXFVTwmNSqTbHlO/\n"
    "R/3c9xZSpfmzA1ixBQiS3ZFBG/0PdiqB75aE5Xe4JzgfZP73NKgV8N2SI+EeiG7A\n"
    "gfAkT1fO96E3/o988UH3YWuzK4AQ60tylZ2lfOoVDHiKGZt53S5UZuDvWoGQM5Pv\n"
    "VeTanTM3NP+b8zweuda+osfXGZTVo7x2fmG7eWalCtxcVQYI3j1ARHZwkNEm+4lx\n"
    "1hEBWXw4CCZhhNIuLjP7DSMQksbW7ioyAzqJNTr/UO6vWmfEyfbFJCAutC6KccLA\n"
    "K/9VfrED6APb0aDqAVepEEBjZUhtXc215AgnvtbAQPZ28cC6VCvswgDhOIOUP6wg\n"
    "vkl0YPeZighmfLmmbyJ0k7ksfzkmuNTza73a4ZdokCHV33pWqd0Va3aihQ6J7uOL\n"
    "Ycr0bs2jHUMVzdG+AI+y/UqsnF+zFttFYH7L5SjCFcwoxbia6MghiO5OmhdhxeB6\n"
    "mth8TaG7foaXmbdZQtiBob9I7tl3UPk/JfVpnqEj0wIDAQABMA0GCSqGSIb3DQEB\n"
    "CwUAA4ICAQBljr6tsIuQAHAylJxE+PTyfLRSVrJQiwp9OoJAnInZfw1IEHLCQQNq\n"
    "xgwHGGArZSnG5q2ccQV1Xn9w50bIkd3O9FJEfLUdPBSUBGwaZSUKcai8UFognuMP\n"
    "XS0eZ9/1eCewDanyZY0dgmTAAn5++upnJUa6bs2MRpFlRm+NKeqSUkl0zw2uv/EZ\n"
    "yHR6CFJQo7WD8OG1V4WhdltoayP8bGMX7KCweQdk48TWHCchyLO2maIg9+ZYSkRR\n"
    "Rt0Vjpi114Cu77QWHoP4T8SbBiBilMcYfo3VO+mRabGLip8qs6NvoEI8bDMhYm3A\n"
    "1stQHOvf7zD/vOihZ6J3BsO+SOLxmxv7lyuxeShrNmoUOkcYX/eu3j/zGTArqzfU\n"
    "TKCWTZymOth3SOeP8H8STU9LWwLON4gVq8sRB+17IrJ9iB5Xj1BWBd4+xHdHY3rA\n"
    "+ghQsE/VZbzBw+Qll/Rz8fqCB8+ecZILhZKpCugB2KY9Kfs40pyKo6Zm5MSKOZvn\n"
    "GXZwhwPco4pa35mJZZVk764uYeyMLOb0cdJOyb+oXfj4IVjk0HuVq36vSrB+1npa\n"
    "M10OXlIQm5eAw2grGPksIAuD5DbL3Xag3bOmPm1zgLbxVtNJ9QuKmjEgU2AaxxQw\n"
    "QtdaNZsFnsTwlGcsOQ475KcGZo1+CiceLj0RBsaAeQNLTwTRJjNcMA==\n"
    "-----END CERTIFICATE-----\n";

static const char fio___pem_test_key_pem[] =
    "-----BEGIN PRIVATE KEY-----\n"
    "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\n"
    "0qZTWhC94/EdDbDzbbLibZ08UB0VBte4xw==\n"
    "-----END PRIVATE KEY-----\n";

#define FIO___PEM_TEST_CERT_LEN (sizeof(fio___pem_test_cert_pem) - 1)
#define FIO___PEM_TEST_KEY_LEN  (sizeof(fio___pem_test_key_pem) - 1)

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
  const char *pem = fio___pem_test_cert_pem;
  size_t pem_len = FIO___PEM_TEST_CERT_LEN;

  fio_x509_cert_s cert;
  FIO_ASSERT(fio_pem_parse_certificate(&cert, pem, pem_len) == 0,
             "failed to parse embedded certificate PEM");
  FIO_ASSERT(cert.key_type == FIO_X509_KEY_RSA,
             "embedded certificate should contain an RSA key");
  FIO_ASSERT(cert.pubkey.rsa.n_len >= 256,
             "RSA modulus should be at least 256 bytes");
  FIO_ASSERT(cert.pubkey.rsa.e_len == 3,
             "RSA exponent should be 3 bytes");
  FIO_ASSERT(cert.subject_cn != NULL, "subject CN missing");
  FIO_ASSERT(fio_x509_verify_signature(&cert, &cert) == 0,
             "self-signed embedded certificate signature should verify");
}

FIO_SFUNC void FIO_NAME_TEST(stl, pem_get_certificate_der)(void) {
  const char *pem = fio___pem_test_cert_pem;
  size_t pem_len = FIO___PEM_TEST_CERT_LEN;

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
}

FIO_SFUNC void FIO_NAME_TEST(stl, pem_parse_private_key_pkcs8)(void) {
  const char *pem = fio___pem_test_key_pem;
  size_t pem_len = FIO___PEM_TEST_KEY_LEN;

  fio_pem_private_key_s key;
  FIO_ASSERT(fio_pem_parse_private_key(&key, pem, pem_len) == 0,
             "failed to parse embedded key PEM");
  FIO_ASSERT(key.type == FIO_PEM_KEY_RSA,
             "embedded key should be RSA");
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

FIO_SFUNC EVP_PKEY *fio___pem_test_load_openssl_key(const char *pem,
                                                     size_t pem_len) {
  BIO *bio = BIO_new_mem_buf(pem, (int)pem_len);
  FIO_ASSERT(bio, "BIO_new_mem_buf failed for OpenSSL private key");
  EVP_PKEY *pkey = PEM_read_bio_PrivateKey(bio, NULL, NULL, NULL);
  BIO_free(bio);
  FIO_ASSERT(pkey, "OpenSSL failed to load private key");
  return pkey;
}

FIO_SFUNC void FIO_NAME_TEST(stl, pem_openssl_key_compare)(void) {
  const char *pem = fio___pem_test_key_pem;
  size_t pem_len = FIO___PEM_TEST_KEY_LEN;

  fio_pem_private_key_s fkey;
  FIO_ASSERT(fio_pem_parse_private_key(&fkey, pem, pem_len) == 0,
             "fio failed to parse embedded key PEM");

  EVP_PKEY *pkey = fio___pem_test_load_openssl_key(pem, pem_len);
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
}

FIO_SFUNC void FIO_NAME_TEST(stl, pem_openssl_cert_compare)(void) {
  const char *pem = fio___pem_test_cert_pem;
  size_t pem_len = FIO___PEM_TEST_CERT_LEN;

  fio_x509_cert_s fc;
  FIO_ASSERT(fio_pem_parse_certificate(&fc, pem, pem_len) == 0,
             "fio failed to parse embedded certificate PEM");

  BIO *bio = BIO_new_mem_buf(pem, (int)pem_len);
  FIO_ASSERT(bio, "BIO_new_mem_buf failed");
  X509 *x = PEM_read_bio_X509(bio, NULL, NULL, NULL);
  BIO_free(bio);
  FIO_ASSERT(x, "OpenSSL failed to parse embedded certificate PEM");

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
}

FIO_SFUNC void FIO_NAME_TEST(stl, pem_roundtrip_sign_verify)(void) {
  const char *pem = fio___pem_test_key_pem;
  size_t pem_len = FIO___PEM_TEST_KEY_LEN;

  fio_pem_private_key_s fkey;
  FIO_ASSERT(fio_pem_parse_private_key(&fkey, pem, pem_len) == 0,
             "fio failed to parse embedded key PEM for roundtrip");

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
