/* *****************************************************************************
Test for 157 pem.h

Coverage: PEM block parsing, X.509 certificate loading from PEM, and private
key loading (PKCS#8, PKCS#1, SEC1, P-256 and P-384, incl. compressed embedded
public keys and curve / length mismatches). Uses embedded RSA certificate and
private-key PEM fixtures as inputs, and uses OpenSSL as a library to
cross-check parsed key material. Also includes edge cases for malformed PEM
and unsupported labels.
***************************************************************************** */
#include "test-helpers.h"

#define FIO_TIME
#define FIO_SHA2
#define FIO_MATH
#define FIO_DER
#define FIO_RSA
#define FIO_ED25519
#define FIO_P256
#define FIO_P384
#define FIO_STR
#define FIO_X509
#define FIO_PEM
#include FIO_INCLUDE_FILE

#include "ec-compressed-fixtures.h"

#if HAVE_OPENSSL
#include <openssl/bio.h>
#include <openssl/bn.h>
#include <openssl/ec.h>
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
      "dXNlcnMwXDANBgkqhkiG9w0BAQEFAANLADBIAkEA\n"
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
  FIO_ASSERT(cert.key_algo == FIO_X509_KEY_RSA,
             "embedded certificate should contain an RSA key");
  FIO_ASSERT(cert.pubkey.rsa.n.len >= 256,
             "RSA modulus should be at least 256 bytes");
  FIO_ASSERT(cert.pubkey.rsa.e.len == 3,
             "RSA exponent should be 3 bytes");
  FIO_ASSERT(cert.cn.buf != NULL, "subject CN missing");
  FIO_ASSERT(fio_x509_verify_signature(&cert, &cert) == 0,
             "self-signed embedded certificate signature should verify");
  fio_pem_certificate_free(&cert);
  FIO_ASSERT(!cert.der.buf && !cert.der.len,
             "fio_pem_certificate_free should reset the certificate");
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
  FIO_ASSERT(cert1.pubkey.rsa.n.len == cert2.pubkey.rsa.n.len,
             "DER/PEM parse mismatch");
  FIO_ASSERT(!FIO_MEMCMP(cert1.pubkey.rsa.n.buf, cert2.pubkey.rsa.n.buf,
                         cert1.pubkey.rsa.n.len),
             "modulus mismatch between DER and PEM parse");
  fio_pem_certificate_free(&cert2);
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
Strict framing / Base64 / consumption tests
***************************************************************************** */

/* OpenSSL-generated P-256 (SEC1 + PKCS#8) and Ed25519 (PKCS#8) fixtures */
static const char fio___pem_test_ec_sec1[] =
    "-----BEGIN EC PRIVATE KEY-----\n"
    "MHcCAQEEIEs6hZiOr11cKJLA77i76xjxPT8ogFSjaBVDs/yi0LuRoAoGCCqGSM49\n"
    "AwEHoUQDQgAEwdkt/5yKm3TJx8mWDMC/JIBgTCpL1nuXFMuF76KIptX5bEZEBHKU\n"
    "XMntLoT+JpahuuvZ676rWaGixrxRAHhu9g==\n"
    "-----END EC PRIVATE KEY-----\n";
static const char fio___pem_test_ec_pkcs8[] =
    "-----BEGIN PRIVATE KEY-----\n"
    "MIGHAgEAMBMGByqGSM49AgEGCCqGSM49AwEHBG0wawIBAQQgSzqFmI6vXVwoksDv\n"
    "uLvrGPE9PyiAVKNoFUOz/KLQu5GhRANCAATB2S3/nIqbdMnHyZYMwL8kgGBMKkvW\n"
    "e5cUy4Xvooim1flsRkQEcpRcye0uhP4mlqG669nrvqtZoaLGvFEAeG72\n"
    "-----END PRIVATE KEY-----\n";
static const char fio___pem_test_ed25519_pkcs8[] =
    "-----BEGIN PRIVATE KEY-----\n"
    "MC4CAQAwBQYDK2VwBCIEIBi9fN0g8t7Fmg1Zf5mQyTZGiG24xh1ji1JlgurO8aV5\n"
    "-----END PRIVATE KEY-----\n";

FIO_SFUNC void FIO_NAME_TEST(stl, pem_consumed_offsets)(void) {
  static const char *eols[] = {"\n", "\r\n", "  \t\n", "\r\n\r\n"};
  uint8_t der[64];
  fio_pem_s block;
  for (size_t i = 0; i < sizeof(eols) / sizeof(eols[0]); ++i) {
    char buf[256];
    int len = snprintf(buf,
                       sizeof(buf),
                       "-----BEGIN A-----%sAQID%s-----END A-----%s"
                       "-----BEGIN B-----%sBAUG%s-----END B-----%s",
                       eols[i],
                       eols[i],
                       eols[i],
                       eols[i],
                       eols[i],
                       eols[i]);
    const char *second = strstr(buf, "-----BEGIN B");
    size_t consumed = fio_pem_parse(&block, der, sizeof(der), buf, (size_t)len);
    FIO_ASSERT(consumed, "first block parse failed (%zu)", i);
    /* consumption runs through the END marker and trailing whitespace */
    const char *expect = strstr(buf, "-----END A-----") + 15;
    while (*expect == '\r' || *expect == '\n' || *expect == ' ' ||
           *expect == '\t')
      ++expect;
    FIO_ASSERT(consumed == (size_t)(expect - buf),
               "consumed offset mismatch (%zu): %zu != %zu",
               i,
               consumed,
               (size_t)(expect - buf));
    FIO_ASSERT(block.der_len == 3 && der[0] == 1 && der[2] == 3,
               "first block DER mismatch");
    FIO_ASSERT(expect == second, "consumption overran next block");
    size_t c2 = fio_pem_parse(&block,
                              der,
                              sizeof(der),
                              buf + consumed,
                              (size_t)len - consumed);
    FIO_ASSERT(c2 == (size_t)len - consumed, "second block not fully consumed");
    FIO_ASSERT(block.label_len == 1 && block.label[0] == 'B' &&
                   block.der_len == 3 && der[0] == 4,
               "second block mismatch");
  }
}

FIO_SFUNC void FIO_NAME_TEST(stl, pem_strict_base64)(void) {
  static const struct {
    const char *body;
    int ok;
    size_t len;
  } v[] = {
      {"", 1, 0},
      {"AA==", 1, 1},
      {"AAA=", 1, 2},
      {"AQID", 1, 3},
      {"AQ ID\r\n\tBA==", 1, 4},
      {"AQID\n\n\v\f\nBA==\n\n", 1, 4}, /* blank lines, any whitespace */
      {"AB==", 1, 1},                     /* non-zero trailing bits ignored */
      {"AAB=", 1, 2},                     /* non-zero trailing bits ignored */
      {"AAA", 1, 2},                      /* missing padding */
      {"AA", 1, 1},                       /* missing padding */
      {"AA=", 1, 1},                      /* short padding */
      {"AA===", 1, 1},                    /* excess padding */
      {"AAAA=", 1, 3},                    /* padding after a full quantum */
      {"==", 1, 0},                       /* padding only */
      {"+/-_", 1, 3},                     /* URL-safe alphabet == standard */
      {"A", 0, 0},                        /* 6 bits cannot form a byte */
      {"A===", 0, 0},                     /* 6 bits cannot form a byte */
      {"AA=A", 0, 0},                     /* data after padding */
      {"AA==AAAA", 0, 0},                 /* data after padding */
      {"AQID!", 0, 0},                    /* undecodable byte */
      {"Proc-Type: 4,ENCRYPTED\nAQID", 0, 0}, /* encrypted PEM headers */
  };
  uint8_t der[16];
  fio_pem_s block;
  for (size_t i = 0; i < sizeof(v) / sizeof(v[0]); ++i) {
    char buf[256];
    int len = snprintf(buf,
                       sizeof(buf),
                       "-----BEGIN X-----\n%s\n-----END X-----\n",
                       v[i].body);
    size_t r = fio_pem_parse(&block, der, sizeof(der), buf, (size_t)len);
    FIO_ASSERT(!!r == v[i].ok, "base64 vector %zu (%s) wrong", i, v[i].body);
    if (r)
      FIO_ASSERT(block.der_len == v[i].len, "base64 vector %zu length", i);
  }
  /* exact output capacity is enough; one byte less is an error */
  const char *pem = "-----BEGIN X-----\nAQID\n-----END X-----\n";
  FIO_ASSERT(fio_pem_parse(&block, der, 3, pem, FIO_STRLEN(pem)),
             "exact-size output buffer should suffice");
  FIO_ASSERT(!fio_pem_parse(&block, der, 2, pem, FIO_STRLEN(pem)),
             "short output buffer should fail");
  /* URL-safe and standard alphabets decode identically */
  {
    uint8_t d2[16];
    const char *a1 = "-----BEGIN X-----\n+/+/\n-----END X-----\n";
    const char *a2 = "-----BEGIN X-----\n-_-_\n-----END X-----\n";
    FIO_ASSERT(fio_pem_parse(&block, der, sizeof(der), a1, FIO_STRLEN(a1)) &&
                   fio_pem_parse(&block, d2, sizeof(d2), a2, FIO_STRLEN(a2)) &&
                   !FIO_MEMCMP(der, d2, 3) && der[0] == 0xFB,
               "URL-safe alphabet should match standard alphabet");
  }
  /* label: any single-line text; a label that spans lines is a broken BEGIN */
  const char *bad_label = "-----BEGIN X\nY-----\nAQID\n-----END X\nY-----\n";
  FIO_ASSERT(!fio_pem_parse(&block, der, sizeof(der), bad_label,
                            FIO_STRLEN(bad_label)),
             "label with newline should fail");
  /* explanatory text around the block and unusual single-line labels */
  const char *wrapped = "Subject: test\n-----BEGIN X509\tCRL v2-----\nAQID\n"
                        "-----END X509\tCRL v2-----\ntrailing text\n";
  FIO_ASSERT(fio_pem_parse(&block, der, sizeof(der), wrapped,
                           FIO_STRLEN(wrapped)) &&
                 block.label_len == 11 && block.der_len == 3,
             "surrounding text / unusual label should be accepted");
}

/* *****************************************************************************
Private key schema tests (exact consumption / field counts / values)
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, pem_key_fixtures)(void) {
  fio_pem_private_key_s k1, k2, k3;
  FIO_ASSERT(!fio_pem_parse_private_key(&k1,
                                        fio___pem_test_ec_sec1,
                                        FIO_STRLEN(fio___pem_test_ec_sec1)),
             "SEC1 P-256 key failed");
  FIO_ASSERT(!fio_pem_parse_private_key(&k2,
                                        fio___pem_test_ec_pkcs8,
                                        FIO_STRLEN(fio___pem_test_ec_pkcs8)),
             "PKCS#8 P-256 key failed");
  FIO_ASSERT(k1.type == FIO_PEM_KEY_ECDSA_P256 &&
                 k2.type == FIO_PEM_KEY_ECDSA_P256,
             "P-256 key type");
  FIO_ASSERT(k1.ecdsa_p256.has_public_key && k2.ecdsa_p256.has_public_key &&
                 k1.ecdsa_p256.public_key_len == 65 &&
                 k2.ecdsa_p256.public_key_len == 65,
             "P-256 public key should be imported");
  FIO_ASSERT(!FIO_MEMCMP(&k1.ecdsa_p256, &k2.ecdsa_p256, 32 + 65),
             "SEC1 / PKCS#8 P-256 mismatch");
  uint8_t pub[65];
  FIO_ASSERT(!fio_p256_public_key(pub, k1.ecdsa_p256.private_key) &&
                 !FIO_MEMCMP(pub, k1.ecdsa_p256.public_key, 65),
             "embedded P-256 public key should match the derived one");
  FIO_ASSERT(
      !fio_pem_parse_private_key(&k3,
                                 fio___pem_test_ed25519_pkcs8,
                                 FIO_STRLEN(fio___pem_test_ed25519_pkcs8)) &&
          k3.type == FIO_PEM_KEY_ED25519 && k3.ed25519.private_key[0] == 0x18 &&
          k3.ed25519.private_key[31] == 0x79 && !k3.ed25519.has_public_key,
      "Ed25519 PKCS#8 key failed");
  fio_pem_private_key_clear(&k1);
  fio_pem_private_key_clear(&k2);
  fio_pem_private_key_clear(&k3);

  /* PKCS#1 (legacy RSA) == the RSAPrivateKey inside the PKCS#8 fixture */
  uint8_t der[4096];
  fio_pem_s block;
  FIO_ASSERT(fio_pem_parse(&block, der, sizeof(der), fio___pem_test_key_pem,
                           FIO___PEM_TEST_KEY_LEN),
             "PKCS#8 RSA decode failed");
  fio_der_element_s seq, e;
  fio_der_iterator_s it;
  fio_der_parse(&seq, block.der, block.der_len);
  fio_der_iterator_init(&it, &seq);
  fio_der_iterator_next(&it, &e); /* version */
  fio_der_iterator_next(&it, &e); /* algorithm */
  fio_der_iterator_next(&it, &e); /* privateKey */
  FIO_ASSERT(e.tag == 0x04, "PKCS#8 privateKey OCTET STRING expected");
  FIO_MEMSET(&k1, 0, sizeof(k1));
  FIO_ASSERT(!fio___pem_parse_rsa_private_key(&k1, e.data, e.len),
             "PKCS#1 RSA parse failed");
  FIO_ASSERT(!fio_pem_parse_private_key(&k2, fio___pem_test_key_pem,
                                        FIO___PEM_TEST_KEY_LEN),
             "PKCS#8 RSA parse failed");
  FIO_ASSERT(!FIO_MEMCMP(&k1, &k2, sizeof(k1)), "PKCS#1 / PKCS#8 mismatch");
  /* trailing byte after the RSAPrivateKey must be rejected */
  FIO_ASSERT(fio___pem_parse_rsa_private_key(&k1, e.data, e.len + 1),
             "trailing data after RSAPrivateKey should fail");
  fio_pem_private_key_clear(&k1);
  fio_pem_private_key_clear(&k2);
}

/* P-256 keys whose embedded public key is compressed (SEC 1: 02/03 || x) */
FIO_SFUNC void FIO_NAME_TEST(stl, pem_compressed_ec_keys)(void) {
  static const char *pems[] = {fio___test_ecc_p256_key,
                               fio___test_ecc_p256_key_pkcs8,
                               fio___test_ecc_p256_key_badpub};
  static const uint8_t zero[32] = {0};
  fio_pem_private_key_s k[3];
  uint8_t pub[65], comp[33];
  for (size_t i = 0; i < 3; ++i) {
    FIO_ASSERT(!fio_pem_parse_private_key(k + i, pems[i], FIO_STRLEN(pems[i])),
               "compressed P-256 key %zu should parse",
               i);
    FIO_ASSERT(k[i].type == FIO_PEM_KEY_ECDSA_P256 &&
                   k[i].ecdsa_p256.has_public_key &&
                   k[i].ecdsa_p256.public_key_len == 33 &&
                   (k[i].ecdsa_p256.public_key[0] == 0x02 ||
                    k[i].ecdsa_p256.public_key[0] == 0x03),
               "compressed P-256 key %zu: 33 byte public key expected",
               i);
    FIO_ASSERT(!FIO_MEMCMP(k[i].ecdsa_p256.public_key + 33, zero, 32),
               "compressed P-256 key %zu: unused public key bytes not zero",
               i);
  }
  FIO_ASSERT(!FIO_MEMCMP(&k[0].ecdsa_p256, &k[1].ecdsa_p256, 32 + 65) &&
                 k[1].ecdsa_p256.public_key_len == 33,
             "compressed SEC1 / PKCS#8 P-256 mismatch");
  FIO_ASSERT(!fio_p256_public_key(pub, k[0].ecdsa_p256.private_key),
             "P-256 public key derivation failed");
  comp[0] = (uint8_t)(0x02 | (pub[64] & 1));
  FIO_MEMCPY(comp + 1, pub + 1, 32);
  FIO_ASSERT(comp[0] == 0x03 &&
                 !FIO_MEMCMP(comp, k[0].ecdsa_p256.public_key, 33),
             "embedded compressed public key should match the derived one");
  /* "badpub": same scalar, the CA's compressed point (parses; TLS rejects) */
  FIO_ASSERT(!FIO_MEMCMP(k[2].ecdsa_p256.private_key,
                         k[0].ecdsa_p256.private_key,
                         32) &&
                 k[2].ecdsa_p256.public_key[0] == 0x02 &&
                 FIO_MEMCMP(comp + 1, k[2].ecdsa_p256.public_key + 1, 32),
             "badpub fixture should embed a different public key");
  for (size_t i = 0; i < 3; ++i)
    fio_pem_private_key_clear(k + i);
}

/* P-384 keys: SEC1 / PKCS#8, embedded public key compressed or uncompressed */
FIO_SFUNC void FIO_NAME_TEST(stl, pem_p384_keys)(void) {
  static const char *pems[] = {fio___test_ecc_p384_key,
                               fio___test_ecc_p384_key_pkcs8,
                               fio___test_ecc_p384u_key,
                               fio___test_ecc_p384u_key_pkcs8,
                               fio___test_ecc_p384_key_badpub};
  static const size_t pub_lens[] = {49, 49, 97, 97, 49};
  static const uint8_t zero[48] = {0};
  fio_pem_private_key_s k[5];
  uint8_t pub[97], comp[49];
  for (size_t i = 0; i < 5; ++i) {
    FIO_ASSERT(!fio_pem_parse_private_key(k + i, pems[i], FIO_STRLEN(pems[i])),
               "P-384 key %zu should parse",
               i);
    FIO_ASSERT(k[i].type == FIO_PEM_KEY_ECDSA_P384 &&
                   k[i].ecdsa_p384.has_public_key &&
                   k[i].ecdsa_p384.public_key_len == pub_lens[i],
               "P-384 key %zu: type / %zu byte public key expected",
               i,
               pub_lens[i]);
    FIO_ASSERT(pub_lens[i] == 97 ||
                   !FIO_MEMCMP(k[i].ecdsa_p384.public_key + 49, zero, 48),
               "P-384 key %zu: unused public key bytes not zero",
               i);
  }
  /* SEC1 == PKCS#8 (scalar, public key and its length) per key */
  for (size_t i = 0; i < 4; i += 2)
    FIO_ASSERT(!FIO_MEMCMP(&k[i].ecdsa_p384,
                           &k[i + 1].ecdsa_p384,
                           sizeof(k[i].ecdsa_p384)),
               "P-384 SEC1 / PKCS#8 mismatch (%zu)",
               i);
  /* the embedded public keys match the derived ones (both forms) */
  FIO_ASSERT(!fio_p384_public_key(pub, k[2].ecdsa_p384.private_key) &&
                 !FIO_MEMCMP(pub, k[2].ecdsa_p384.public_key, 97),
             "embedded uncompressed P-384 public key should match");
  FIO_ASSERT(!fio_p384_public_key(pub, k[0].ecdsa_p384.private_key),
             "P-384 public key derivation failed");
  comp[0] = (uint8_t)(0x02 | (pub[96] & 1));
  FIO_MEMCPY(comp + 1, pub + 1, 48);
  FIO_ASSERT(comp[0] == 0x03 &&
                 !FIO_MEMCMP(comp, k[0].ecdsa_p384.public_key, 49),
             "embedded compressed P-384 public key should match");
  /* "badpub": same scalar, the CA's compressed point (parses; TLS rejects) */
  FIO_ASSERT(!FIO_MEMCMP(k[4].ecdsa_p384.private_key,
                         k[0].ecdsa_p384.private_key,
                         48) &&
                 k[4].ecdsa_p384.public_key[0] == 0x02 &&
                 FIO_MEMCMP(comp + 1, k[4].ecdsa_p384.public_key + 1, 48),
             "P-384 badpub fixture should embed a different public key");
  for (size_t i = 0; i < 5; ++i)
    fio_pem_private_key_clear(k + i);

  /* --- crafted SEC1 / PKCS#8 structures --- */
  fio_pem_private_key_s key;
  static const uint8_t oid384[] = {0x06, 0x05, 0x2B, 0x81, 0x04, 0x00, 0x22};
  static const uint8_t oid256[] = {0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE,
                                   0x3D, 0x03, 0x01, 0x07};
  uint8_t ec[192], *ecp = ec, p8[192];
  /* SEC1: version, scalar (`slen` bytes), optional [0] curve, optional [1] */
#define FIO___PEM_P384_SEC1(slen, curve_oid, curve_len, pt_len)                \
  do {                                                                         \
    size_t l_ = 3; /* room for a long-form SEQUENCE header */                  \
    ec[l_++] = 0x02, ec[l_++] = 1, ec[l_++] = 1;                               \
    ec[l_++] = 0x04, ec[l_++] = (uint8_t)(slen);                               \
    for (size_t i_ = 0; i_ < (size_t)(slen); ++i_)                             \
      ec[l_++] = (uint8_t)(i_ + 1);                                            \
    if (curve_len) {                                                           \
      ec[l_++] = 0xA0, ec[l_++] = (uint8_t)(curve_len);                        \
      FIO_MEMCPY(ec + l_, curve_oid, curve_len);                               \
      l_ += curve_len;                                                         \
    }                                                                          \
    if (pt_len) {                                                              \
      ec[l_++] = 0xA1, ec[l_++] = (uint8_t)((pt_len) + 3);                     \
      ec[l_++] = 0x03, ec[l_++] = (uint8_t)((pt_len) + 1), ec[l_++] = 0;       \
      ec[l_++] = (uint8_t)((pt_len) == 65 || (pt_len) == 97 ? 0x04 : 0x03);     \
      for (int i_ = 1; i_ < (int)(pt_len); ++i_)                                \
        ec[l_++] = (uint8_t)(0x80 + i_);                                       \
    }                                                                          \
    if (l_ - 3 < 0x80) {                                                       \
      ec[1] = 0x30, ec[2] = (uint8_t)(l_ - 3), ecp = ec + 1, ec_len = l_ - 1;  \
    } else {                                                                   \
      ec[0] = 0x30, ec[1] = 0x81, ec[2] = (uint8_t)(l_ - 3);                   \
      ecp = ec, ec_len = l_;                                                   \
    }                                                                          \
  } while (0)
  size_t ec_len = 0;
  /* legacy SEC1 without parameters: the 48 byte scalar implies P-384 */
  FIO___PEM_P384_SEC1(48, oid384, 0, 0);
  FIO_ASSERT(!fio___pem_parse_ec_private_key(&key,
                                             ecp,
                                             ec_len,
                                             FIO_PEM_KEY_UNKNOWN) &&
                 key.type == FIO_PEM_KEY_ECDSA_P384 &&
                 key.ecdsa_p384.private_key[0] == 1 &&
                 key.ecdsa_p384.private_key[47] == 48 &&
                 !key.ecdsa_p384.has_public_key &&
                 !key.ecdsa_p384.public_key_len,
             "minimal P-384 SEC1 key should parse");
  FIO___PEM_P384_SEC1(48, oid384, sizeof(oid384), 97);
  FIO_ASSERT(!fio___pem_parse_ec_private_key(&key,
                                             ecp,
                                             ec_len,
                                             FIO_PEM_KEY_UNKNOWN) &&
                 key.type == FIO_PEM_KEY_ECDSA_P384 &&
                 key.ecdsa_p384.public_key_len == 97 &&
                 key.ecdsa_p384.public_key[0] == 0x04 &&
                 key.ecdsa_p384.public_key[96] == 0x80 + 96,
             "P-384 SEC1 key with an uncompressed public key should parse");
  FIO___PEM_P384_SEC1(48, oid384, sizeof(oid384), 49);
  FIO_ASSERT(!fio___pem_parse_ec_private_key(&key,
                                             ecp,
                                             ec_len,
                                             FIO_PEM_KEY_ECDSA_P384) &&
                 key.type == FIO_PEM_KEY_ECDSA_P384 &&
                 key.ecdsa_p384.public_key_len == 49 &&
                 key.ecdsa_p384.public_key[0] == 0x03 &&
                 key.ecdsa_p384.public_key[48] == 0x80 + 48,
             "P-384 SEC1 key with a compressed public key should parse");
  { /* the unused tail of a compressed public key is zeroed */
    static const uint8_t zero48[48] = {0};
    FIO_MEMSET(&key, 0xFF, sizeof(key));
    FIO_ASSERT(!fio___pem_parse_ec_private_key(&key,
                                               ecp,
                                               ec_len,
                                               FIO_PEM_KEY_UNKNOWN) &&
                   !FIO_MEMCMP(key.ecdsa_p384.public_key + 49, zero48, 48),
               "P-384 compressed public key: unused bytes must be zeroed");
  }
  /* curve / length disagreements */
  FIO___PEM_P384_SEC1(32, oid384, 0, 0);
  FIO_ASSERT(fio___pem_parse_ec_private_key(&key,
                                            ecp,
                                            ec_len,
                                            FIO_PEM_KEY_ED25519),
             "a non-EC curve hint must fail (no parameters)");
  FIO___PEM_P384_SEC1(48, oid384, sizeof(oid384), 0);
  FIO_ASSERT(fio___pem_parse_ec_private_key(&key,
                                            ecp,
                                            ec_len,
                                            FIO_PEM_KEY_ECDSA_P256),
             "secp384r1 parameters under a P-256 AlgorithmIdentifier must fail");
  FIO_ASSERT(fio___pem_parse_ec_private_key(&key,
                                            ecp,
                                            ec_len,
                                            FIO_PEM_KEY_ED25519),
             "a non-EC curve hint must fail");
  FIO___PEM_P384_SEC1(32, oid256, sizeof(oid256), 0);
  FIO_ASSERT(fio___pem_parse_ec_private_key(&key,
                                            ecp,
                                            ec_len,
                                            FIO_PEM_KEY_ECDSA_P384),
             "secp256r1 parameters under a P-384 AlgorithmIdentifier must fail");
  FIO_ASSERT(!fio___pem_parse_ec_private_key(&key,
                                             ecp,
                                             ec_len,
                                             FIO_PEM_KEY_ECDSA_P256) &&
                 key.type == FIO_PEM_KEY_ECDSA_P256,
             "matching P-256 parameters and hint should parse");
  FIO___PEM_P384_SEC1(32, oid384, sizeof(oid384), 0);
  FIO_ASSERT(fio___pem_parse_ec_private_key(&key,
                                            ecp,
                                            ec_len,
                                            FIO_PEM_KEY_UNKNOWN),
             "32 byte scalar with secp384r1 parameters must fail");
  FIO___PEM_P384_SEC1(48, oid256, sizeof(oid256), 0);
  FIO_ASSERT(fio___pem_parse_ec_private_key(&key,
                                            ecp,
                                            ec_len,
                                            FIO_PEM_KEY_UNKNOWN),
             "48 byte scalar with secp256r1 parameters must fail");
  FIO___PEM_P384_SEC1(47, oid384, sizeof(oid384), 0);
  FIO_ASSERT(fio___pem_parse_ec_private_key(&key,
                                            ecp,
                                            ec_len,
                                            FIO_PEM_KEY_UNKNOWN),
             "47 byte P-384 scalar must fail");
  FIO___PEM_P384_SEC1(49, oid384, sizeof(oid384), 0);
  FIO_ASSERT(fio___pem_parse_ec_private_key(&key,
                                            ecp,
                                            ec_len,
                                            FIO_PEM_KEY_UNKNOWN),
             "49 byte P-384 scalar must fail");
  FIO___PEM_P384_SEC1(48, oid384, 0, 0);
  FIO_ASSERT(fio___pem_parse_ec_private_key(&key,
                                            ecp,
                                            ec_len,
                                            FIO_PEM_KEY_ECDSA_P256),
             "48 byte scalar under a P-256 AlgorithmIdentifier must fail");
  FIO___PEM_P384_SEC1(32, oid384, 0, 0);
  FIO_ASSERT(fio___pem_parse_ec_private_key(&key,
                                            ecp,
                                            ec_len,
                                            FIO_PEM_KEY_ECDSA_P384),
             "32 byte scalar under a P-384 AlgorithmIdentifier must fail");
  FIO___PEM_P384_SEC1(40, oid384, 0, 0);
  FIO_ASSERT(fio___pem_parse_ec_private_key(&key,
                                            ecp,
                                            ec_len,
                                            FIO_PEM_KEY_UNKNOWN),
             "40 byte scalar without parameters must fail");
  /* public key length must follow the curve */
  {
    static const size_t bad_pts[] = {65, 33, 96, 48, 98, 50};
    for (size_t j = 0; j < sizeof(bad_pts) / sizeof(bad_pts[0]); ++j) {
      FIO___PEM_P384_SEC1(48, oid384, sizeof(oid384), bad_pts[j]);
      FIO_ASSERT(fio___pem_parse_ec_private_key(&key,
                                                ecp,
                                                ec_len,
                                                FIO_PEM_KEY_UNKNOWN),
                 "%zu byte public key on P-384 must fail",
                 bad_pts[j]);
    }
    FIO___PEM_P384_SEC1(32, oid256, sizeof(oid256), 97);
    FIO_ASSERT(fio___pem_parse_ec_private_key(&key,
                                              ecp,
                                              ec_len,
                                              FIO_PEM_KEY_UNKNOWN),
               "97 byte public key on P-256 must fail");
    FIO___PEM_P384_SEC1(32, oid256, sizeof(oid256), 49);
    FIO_ASSERT(fio___pem_parse_ec_private_key(&key,
                                              ecp,
                                              ec_len,
                                              FIO_PEM_KEY_UNKNOWN),
               "49 byte public key on P-256 must fail");
  }
  /* prefixes: 04 only for 97 bytes, 02/03 only for 49 bytes */
  FIO___PEM_P384_SEC1(48, oid384, sizeof(oid384), 97);
  ecp[ec_len - 97] = 0x02;
  FIO_ASSERT(fio___pem_parse_ec_private_key(&key,
                                            ecp,
                                            ec_len,
                                            FIO_PEM_KEY_UNKNOWN),
             "97 byte P-384 public key without 04 must fail");
  FIO___PEM_P384_SEC1(48, oid384, sizeof(oid384), 49);
  for (uint8_t pfx = 0; pfx < 8; ++pfx) {
    ecp[ec_len - 49] = pfx;
    int r = fio___pem_parse_ec_private_key(&key,
                                           ecp,
                                           ec_len,
                                           FIO_PEM_KEY_UNKNOWN);
    FIO_ASSERT((pfx == 2 || pfx == 3) ? (!r && key.ecdsa_p384.public_key[0] ==
                                                   pfx)
                                      : r,
               "49 byte P-384 public key prefix %u",
               (unsigned)pfx);
  }
  /* PKCS#8: AlgorithmIdentifier { ecPublicKey, secp384r1 } */
  FIO___PEM_P384_SEC1(48, oid384, 0, 0);
  {
    static const uint8_t alg[] = {0x30, 0x10, 0x06, 0x07, 0x2A, 0x86, 0x48,
                                  0xCE, 0x3D, 0x02, 0x01, 0x06, 0x05, 0x2B,
                                  0x81, 0x04, 0x00, 0x22};
    size_t l = 0;
    p8[l++] = 0x30, p8[l++] = (uint8_t)(3 + sizeof(alg) + 2 + ec_len);
    p8[l++] = 0x02, p8[l++] = 1, p8[l++] = 0;
    FIO_MEMCPY(p8 + l, alg, sizeof(alg));
    l += sizeof(alg);
    p8[l++] = 0x04, p8[l++] = (uint8_t)ec_len;
    FIO_MEMCPY(p8 + l, ecp, ec_len);
    l += ec_len;
    FIO_ASSERT(l < 0x80 + 2, "fixture size");
    FIO_ASSERT(!fio___pem_parse_pkcs8(&key, p8, l) &&
                   key.type == FIO_PEM_KEY_ECDSA_P384 &&
                   !key.ecdsa_p384.has_public_key &&
                   key.ecdsa_p384.private_key[47] == 48,
               "crafted P-384 PKCS#8 key should parse");
    p8[3 + 2 + 18 + 2 + 6] = 32; /* inner scalar length 48 -> 32 */
    FIO_ASSERT(fio___pem_parse_pkcs8(&key, p8, l),
               "malformed inner SEC1 key must fail");
    p8[3 + 2 + 18 + 2 + 6] = 48;
    p8[3 + 2 + 17] = 0x23; /* secp384r1 -> 1.3.132.0.35 (secp521r1) */
    FIO_ASSERT(fio___pem_parse_pkcs8(&key, p8, l),
               "PKCS#8 secp521r1 must fail");
    p8[3 + 2 + 17] = 0x22;
    FIO_ASSERT(!fio___pem_parse_pkcs8(&key, p8, l), "restored PKCS#8 key");
    /* the AlgorithmIdentifier's curve binds an inner key without parameters:
     * a 32 byte (P-256 sized) scalar under secp384r1 must fail */
    FIO___PEM_P384_SEC1(32, oid384, 0, 0);
    l = 0;
    p8[l++] = 0x30, p8[l++] = (uint8_t)(3 + sizeof(alg) + 2 + ec_len);
    p8[l++] = 0x02, p8[l++] = 1, p8[l++] = 0;
    FIO_MEMCPY(p8 + l, alg, sizeof(alg));
    l += sizeof(alg);
    p8[l++] = 0x04, p8[l++] = (uint8_t)ec_len;
    FIO_MEMCPY(p8 + l, ecp, ec_len);
    l += ec_len;
    FIO_ASSERT(fio___pem_parse_pkcs8(&key, p8, l),
               "32 byte scalar under a secp384r1 PKCS#8 key must fail");
  }
#undef FIO___PEM_P384_SEC1
  fio_pem_private_key_clear(&key);
}

FIO_SFUNC void FIO_NAME_TEST(stl, pem_key_schema)(void) {
  fio_pem_private_key_s key;
  /* --- PKCS#1: 9 INTEGERs, version 0 --- */
  static const uint8_t rsa_ok[] = {
      0x30, 28,                                     /* SEQUENCE */
      0x02, 1, 0,    0x02, 1, 0x7F, 0x02, 1, 3,     /* version, n, e */
      0x02, 1, 5,    0x02, 1, 7,    0x02, 1, 9,     /* d, p, q */
      0x02, 1, 11,   0x02, 2, 0x00, 0x80,           /* dP, dQ */
      0x02, 1, 2};                                  /* qInv */
  FIO_ASSERT(sizeof(rsa_ok) == 30, "fixture size");
  FIO_MEMSET(&key, 0, sizeof(key));
  FIO_ASSERT(!fio___pem_parse_rsa_private_key(&key, rsa_ok, sizeof(rsa_ok)) &&
                 key.rsa.n[0] == 0x7F && key.rsa.dQ_len == 1 &&
                 key.rsa.dQ[0] == 0x80 && key.rsa.qInv_len == 1,
             "minimal PKCS#1 key should parse");
  {
    uint8_t b[64];
    /* extra element inside the SEQUENCE */
    FIO_MEMCPY(b, rsa_ok, sizeof(rsa_ok));
    b[1] += 3;
    b[30] = 0x02, b[31] = 1, b[32] = 1;
    FIO_ASSERT(fio___pem_parse_rsa_private_key(&key, b, 33),
               "extra RSA element (otherPrimeInfos) should fail");
    /* missing coefficient */
    FIO_MEMCPY(b, rsa_ok, sizeof(rsa_ok));
    b[1] -= 3;
    FIO_ASSERT(fio___pem_parse_rsa_private_key(&key, b, 27),
               "missing RSA field should fail");
    /* bad version */
    FIO_MEMCPY(b, rsa_ok, sizeof(rsa_ok));
    b[4] = 1;
    FIO_ASSERT(fio___pem_parse_rsa_private_key(&key, b, sizeof(rsa_ok)),
               "multi-prime / bad RSA version should fail");
    /* negative modulus */
    FIO_MEMCPY(b, rsa_ok, sizeof(rsa_ok));
    b[7] = 0x80;
    FIO_ASSERT(fio___pem_parse_rsa_private_key(&key, b, sizeof(rsa_ok)),
               "negative RSA integer should fail");
    /* zero exponent */
    FIO_MEMCPY(b, rsa_ok, sizeof(rsa_ok));
    b[10] = 0;
    FIO_ASSERT(fio___pem_parse_rsa_private_key(&key, b, sizeof(rsa_ok)),
               "zero RSA integer should fail");
    /* non-minimal (0x00 0x02) */
    FIO_MEMCPY(b, rsa_ok, sizeof(rsa_ok));
    b[26] = 0x02;
    FIO_ASSERT(fio___pem_parse_rsa_private_key(&key, b, sizeof(rsa_ok)),
               "non-minimal RSA integer should fail");
    /* wrong tag (constructed INTEGER) */
    FIO_MEMCPY(b, rsa_ok, sizeof(rsa_ok));
    b[5] = 0x22;
    FIO_ASSERT(fio___pem_parse_rsa_private_key(&key, b, sizeof(rsa_ok)),
               "constructed INTEGER should fail");
    /* trailing byte after the outer SEQUENCE */
    FIO_MEMCPY(b, rsa_ok, sizeof(rsa_ok));
    b[30] = 0;
    FIO_ASSERT(fio___pem_parse_rsa_private_key(&key, b, 31),
               "trailing byte after RSA SEQUENCE should fail");
  }

  /* --- SEC1 ECPrivateKey --- */
  uint8_t ec[128];
  size_t l = 0;
  ec[l++] = 0x30;
  ec[l++] = 0; /* fixed below */
  ec[l++] = 0x02, ec[l++] = 1, ec[l++] = 1;
  ec[l++] = 0x04, ec[l++] = 32;
  for (size_t i = 0; i < 32; ++i)
    ec[l++] = (uint8_t)(i + 1);
  const size_t ec_base = l;
  static const uint8_t ec_params[] = {0xA0, 0x0A, 0x06, 0x08, 0x2A, 0x86,
                                      0x48, 0xCE, 0x3D, 0x03, 0x01, 0x07};
#define FIO___PEM_EC(extra, extra_len)                                         \
  (FIO_MEMCPY(ec + ec_base, extra, extra_len),                                 \
   ec[1] = (uint8_t)(ec_base - 2 + extra_len),                                 \
   fio___pem_parse_ec_private_key(&key,                                        \
                                  ec,                                          \
                                  ec_base + extra_len,                         \
                                  FIO_PEM_KEY_UNKNOWN))
  FIO_ASSERT(!FIO___PEM_EC(ec_params, 0) &&
                 key.type == FIO_PEM_KEY_ECDSA_P256 &&
                 key.ecdsa_p256.private_key[31] == 32,
             "minimal SEC1 key should parse");
  FIO_ASSERT(!FIO___PEM_EC(ec_params, sizeof(ec_params)),
             "SEC1 key with P-256 parameters should parse");
  {
    uint8_t x[96];
    FIO_MEMCPY(x, ec_params, sizeof(ec_params));
    x[11] = 0x08; /* other curve */
    FIO_ASSERT(FIO___PEM_EC(x, sizeof(ec_params)), "wrong curve should fail");
    FIO_MEMCPY(x, ec_params, sizeof(ec_params));
    x[1] = 0x0C, x[12] = 0x05, x[13] = 0x00; /* junk inside [0] */
    FIO_ASSERT(FIO___PEM_EC(x, 14), "trailing data inside [0] should fail");
    x[0] = 0x05, x[1] = 0x00; /* unknown trailing element */
    FIO_ASSERT(FIO___PEM_EC(x, 2), "extra SEC1 element should fail");
    /* public key [1]: BIT STRING, 0 unused bits, 04 || X || Y */
    x[0] = 0xA1, x[1] = 0x44, x[2] = 0x03, x[3] = 0x42, x[4] = 0x00;
    x[5] = 0x04;
    for (size_t i = 6; i < 70; ++i)
      x[i] = (uint8_t)i;
    FIO_ASSERT(!FIO___PEM_EC(x, 70) && key.ecdsa_p256.has_public_key &&
                   key.ecdsa_p256.public_key_len == 65 &&
                   key.ecdsa_p256.public_key[64] == 69,
               "SEC1 key with public key should parse");
    x[4] = 0x01;
    FIO_ASSERT(FIO___PEM_EC(x, 70), "unused bits in public key should fail");
    x[4] = 0x00, x[5] = 0x02;
    FIO_ASSERT(FIO___PEM_EC(x, 70),
               "65 byte public key without 04 should fail");
    /* compressed public key: 02/03 || X (33 bytes) */
    x[1] = 0x24, x[3] = 0x22;
    for (uint8_t pfx = 0; pfx < 8; ++pfx) {
      x[5] = pfx;
      if (pfx == 2 || pfx == 3)
        FIO_ASSERT(!FIO___PEM_EC(x, 38) && key.ecdsa_p256.has_public_key &&
                       key.ecdsa_p256.public_key_len == 33 &&
                       key.ecdsa_p256.public_key[0] == pfx &&
                       key.ecdsa_p256.public_key[32] == 37 &&
                       !key.ecdsa_p256.public_key[33],
                   "SEC1 key with compressed public key should parse");
      else
        FIO_ASSERT(FIO___PEM_EC(x, 38),
                   "33 byte public key with prefix %u should fail",
                   (unsigned)pfx);
    }
    x[5] = 0x02;
    x[1] = 0x23, x[3] = 0x21; /* 32 bytes: truncated compressed point */
    FIO_ASSERT(FIO___PEM_EC(x, 37), "32 byte public key should fail");
    x[1] = 0x44, x[3] = 0x42;
    /* [1] before [0] (out of order) */
    x[5] = 0x04;
    FIO_MEMCPY(x + 70, ec_params, sizeof(ec_params));
    FIO_ASSERT(FIO___PEM_EC(x, 70 + sizeof(ec_params)),
               "out of order SEC1 fields should fail");
  }
  ec[4] = 0; /* version 0 */
  FIO_ASSERT(FIO___PEM_EC(ec_params, 0), "SEC1 version 0 should fail");
  ec[4] = 1;
  FIO_ASSERT(fio___pem_parse_ec_private_key(&key,
                                            ec,
                                            ec_base + 1,
                                            FIO_PEM_KEY_UNKNOWN),
             "trailing byte after SEC1 SEQUENCE should fail");
#undef FIO___PEM_EC

  /* --- PKCS#8 (Ed25519 seed) --- */
  uint8_t p8[128];
  static const uint8_t p8_head[] = {0x30, 0x2E, 0x02, 0x01, 0x00, 0x30,
                                    0x05, 0x06, 0x03, 0x2B, 0x65, 0x70,
                                    0x04, 0x22, 0x04, 0x20};
  FIO_MEMCPY(p8, p8_head, sizeof(p8_head));
  for (size_t i = 0; i < 32; ++i)
    p8[sizeof(p8_head) + i] = (uint8_t)(0xA0 + i);
  const size_t p8_len = sizeof(p8_head) + 32;
  FIO_ASSERT(!fio___pem_parse_pkcs8(&key, p8, p8_len) &&
                 key.type == FIO_PEM_KEY_ED25519 &&
                 key.ed25519.private_key[0] == 0xA0,
             "minimal PKCS#8 Ed25519 key should parse");
  FIO_ASSERT(fio___pem_parse_pkcs8(&key, p8, p8_len + 1),
             "trailing byte after PKCS#8 SEQUENCE should fail");
  {
    uint8_t b[160];
    /* attributes [0] are allowed */
    FIO_MEMCPY(b, p8, p8_len);
    b[1] += 2, b[p8_len] = 0xA0, b[p8_len + 1] = 0x00;
    FIO_ASSERT(!fio___pem_parse_pkcs8(&key, b, p8_len + 2),
               "PKCS#8 attributes should be accepted");
    /* unknown trailing element */
    b[p8_len] = 0x05;
    FIO_ASSERT(fio___pem_parse_pkcs8(&key, b, p8_len + 2),
               "extra PKCS#8 element should fail");
    /* v1 with publicKey [1] -> error; v2 -> ok */
    FIO_MEMCPY(b, p8, p8_len);
    b[1] += 35, b[p8_len] = 0x81, b[p8_len + 1] = 33, b[p8_len + 2] = 0;
    FIO_MEMSET(b + p8_len + 3, 0x55, 32);
    FIO_ASSERT(fio___pem_parse_pkcs8(&key, b, p8_len + 35),
               "PKCS#8 v1 publicKey should fail");
    b[4] = 1;
    FIO_ASSERT(!fio___pem_parse_pkcs8(&key, b, p8_len + 35),
               "PKCS#8 v2 publicKey should parse");
    b[4] = 2;
    FIO_ASSERT(fio___pem_parse_pkcs8(&key, b, p8_len + 35),
               "PKCS#8 version 2 should fail");
    /* Ed25519 AlgorithmIdentifier with parameters */
    static const uint8_t p8_params[] = {0x30, 0x30, 0x02, 0x01, 0x00, 0x30,
                                        0x07, 0x06, 0x03, 0x2B, 0x65, 0x70,
                                        0x05, 0x00, 0x04, 0x22, 0x04, 0x20};
    FIO_MEMCPY(b, p8_params, sizeof(p8_params));
    FIO_MEMSET(b + sizeof(p8_params), 1, 32);
    FIO_ASSERT(fio___pem_parse_pkcs8(&key, b, sizeof(p8_params) + 32),
               "Ed25519 AlgorithmIdentifier parameters should fail");
    /* trailing byte inside the privateKey OCTET STRING */
    FIO_MEMCPY(b, p8, p8_len);
    b[1] += 1, b[13] += 1, b[p8_len] = 0;
    FIO_ASSERT(fio___pem_parse_pkcs8(&key, b, p8_len + 1),
               "trailing data inside CurvePrivateKey should fail");
  }
}

FIO_SFUNC void FIO_NAME_TEST(stl, pem_certificate_lifetime)(void) {
  /* repeated parse / free must not leak (checked by leak counters / ASan) */
  for (size_t i = 0; i < 8; ++i) {
    fio_x509_cert_s cert;
    FIO_ASSERT(!fio_pem_parse_certificate(&cert,
                                          fio___pem_test_cert_pem,
                                          FIO___PEM_TEST_CERT_LEN),
               "certificate parse failed");
    FIO_ASSERT(cert.der.buf && cert.der.len > 0, "cert should own DER");
    fio_pem_certificate_free(&cert);
  }
  fio_x509_cert_s cert;
  FIO_ASSERT(fio_pem_parse_certificate(&cert,
                                       fio___pem_test_key_pem,
                                       FIO___PEM_TEST_KEY_LEN) == -1 &&
                 !cert.der.buf,
             "non-certificate PEM should fail and retain nothing");
  fio_pem_certificate_free(NULL);
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

/* P-384 keys: our parse == OpenSSL's (scalar and uncompressed public point) */
FIO_SFUNC void FIO_NAME_TEST(stl, pem_openssl_p384_compare)(void) {
  static const char *pems[] = {fio___test_ecc_p384_key,
                               fio___test_ecc_p384_key_pkcs8,
                               fio___test_ecc_p384u_key,
                               fio___test_ecc_p384u_key_pkcs8};
  for (size_t i = 0; i < 4; ++i) {
    fio_pem_private_key_s k;
    uint8_t sk[48], pub[97], ours[97];
    FIO_ASSERT(!fio_pem_parse_private_key(&k, pems[i], FIO_STRLEN(pems[i])) &&
                   k.type == FIO_PEM_KEY_ECDSA_P384,
               "P-384 key %zu failed",
               i);
    EVP_PKEY *pkey = fio___pem_test_load_openssl_key(pems[i],
                                                     FIO_STRLEN(pems[i]));
    EC_KEY *ec = EVP_PKEY_get1_EC_KEY(pkey);
    FIO_ASSERT(ec, "OpenSSL: not an EC key (%zu)", i);
    FIO_ASSERT(EC_GROUP_get_curve_name(EC_KEY_get0_group(ec)) ==
                       NID_secp384r1 &&
                   BN_bn2binpad(EC_KEY_get0_private_key(ec), sk, 48) == 48 &&
                   EC_POINT_point2oct(EC_KEY_get0_group(ec),
                                      EC_KEY_get0_public_key(ec),
                                      POINT_CONVERSION_UNCOMPRESSED,
                                      pub,
                                      97,
                                      NULL) == 97,
               "OpenSSL P-384 key export failed (%zu)",
               i);
    FIO_ASSERT(!FIO_MEMCMP(sk, k.ecdsa_p384.private_key, 48),
               "P-384 scalar mismatch with OpenSSL (%zu)",
               i);
    FIO_ASSERT(!fio_p384_public_key(ours, k.ecdsa_p384.private_key) &&
                   !FIO_MEMCMP(ours, pub, 97),
               "P-384 public key mismatch with OpenSSL (%zu)",
               i);
    EC_KEY_free(ec);
    EVP_PKEY_free(pkey);
    fio_secure_zero(sk, sizeof(sk));
    fio_pem_private_key_clear(&k);
  }
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

  FIO_ASSERT(fc.pubkey.rsa.n.len == n_len,
             "cert modulus length mismatch with OpenSSL");
  FIO_ASSERT(fc.pubkey.rsa.e.len == e_len,
             "cert exponent length mismatch with OpenSSL");
  FIO_ASSERT(!FIO_MEMCMP(fc.pubkey.rsa.n.buf, n, n_len),
             "cert modulus mismatch with OpenSSL");
  FIO_ASSERT(!FIO_MEMCMP(fc.pubkey.rsa.e.buf, e, e_len),
             "cert exponent mismatch with OpenSSL");

  RSA_free(rsa);
  EVP_PKEY_free(pkey);
  X509_free(x);
  fio_pem_certificate_free(&fc);
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
  FIO_NAME_TEST(stl, pem_consumed_offsets)();
  FIO_NAME_TEST(stl, pem_strict_base64)();
  FIO_NAME_TEST(stl, pem_key_fixtures)();
  FIO_NAME_TEST(stl, pem_compressed_ec_keys)();
  FIO_NAME_TEST(stl, pem_p384_keys)();
  FIO_NAME_TEST(stl, pem_key_schema)();
  FIO_NAME_TEST(stl, pem_certificate_lifetime)();
#if HAVE_OPENSSL
  FIO_NAME_TEST(stl, pem_openssl_key_compare)();
  FIO_NAME_TEST(stl, pem_openssl_p384_compare)();
  FIO_NAME_TEST(stl, pem_openssl_cert_compare)();
  FIO_NAME_TEST(stl, pem_roundtrip_sign_verify)();
#endif
  return 0;
}
