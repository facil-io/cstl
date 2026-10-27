/* *****************************************************************************
Shared TLS test - server certificate PEM loading fails closed

Include after FIO_INCLUDE_FILE (with FIO_IO and a TLS backend), then call
`fio___test_tls_pem_load(backend_io_functions, "name", password_support)`.

Configured certificate / key files that fail to load (missing file, garbage
PEM, wrong key password, a private key that doesn't match the certificate)
must fail the context build in every backend - never a silent self-signed
substitute. Certificates and keys with compressed EC points (SEC 1) load and
are matched like uncompressed ones; P-384 keys load (both point forms) and
never match a P-256 certificate (or the reverse). Every case is also built as
a client context (client certificates load and fail closed the same way).
Names added without files still get a self-signed certificate (servers).
Fixtures are also used by `tls-connect-verify.h`.

`fio___test_tls_must(fn, "name")` checks that `fio_io_listen` /
`fio_io_connect` fail (instead of running without TLS) when the TLS context
can't be built. Call it before the reactor runs.
***************************************************************************** */
#ifndef H___FIO_TEST_TLS_PEM_LOAD_H
#define H___FIO_TEST_TLS_PEM_LOAD_H
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "ec-compressed-fixtures.h" /* compressed EC point certificates */
#include "mtls-chain-fixtures.h"    /* chains with an intermediate CA */

/* Self-signed P-256 test certificate: CN=localhost, SAN DNS:localhost,
 * CA:FALSE, valid until 2126. */
static const char fio___test_tls_pem_cert[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIBkzCCATigAwIBAgIUGGgRMo9gQJXD1e4twsHLZgmCDyQwCgYIKoZIzj0EAwIw\n"
    "FDESMBAGA1UEAwwJbG9jYWxob3N0MCAXDTI2MTAyODEyMjczNloYDzIxMjYxMDA0\n"
    "MTIyNzM2WjAUMRIwEAYDVQQDDAlsb2NhbGhvc3QwWTATBgcqhkjOPQIBBggqhkjO\n"
    "PQMBBwNCAAQv9fLi4/EUYh3JbQTOKULF63y+fWVLe/SpzZkQAYqomavOrb/1ieJi\n"
    "XICe+KAg4pBzpbpk37KemL4MBqwI3cHBo2YwZDAdBgNVHQ4EFgQUJHSHmnsmKcCs\n"
    "4ZJfLEdil36LdFIwHwYDVR0jBBgwFoAUJHSHmnsmKcCs4ZJfLEdil36LdFIwFAYD\n"
    "VR0RBA0wC4IJbG9jYWxob3N0MAwGA1UdEwEB/wQCMAAwCgYIKoZIzj0EAwIDSQAw\n"
    "RgIhAMc/q8n/t2eKvMy9DnaXv7FkzMBr2zNNxLx8K0nf49OVAiEAl2T543J81Tv6\n"
    "grSph0cp1ztzGCJhjc5nPk1GUxG7fEg=\n"
    "-----END CERTIFICATE-----\n";
/* Its private key (test-only). */
static const char fio___test_tls_pem_key[] =
    "-----BEGIN EC PRIVATE KEY-----\n"
    "MHcCAQEEIHE0vAAX7Ft7NOPJN6mA06TP/F5d3KuigjL4YbLFHogIoAoGCCqGSM49\n"
    "AwEHoUQDQgAEL/Xy4uPxFGIdyW0EzilCxet8vn1lS3v0qc2ZEAGKqJmrzq2/9Yni\n"
    "YlyAnvigIOKQc6W6ZN+ynpi+DAasCN3BwQ==\n"
    "-----END EC PRIVATE KEY-----\n";
/* The same key, PKCS#8 encrypted (AES-256-CBC) with the password "secret". */
static const char fio___test_tls_pem_key_enc[] =
    "-----BEGIN ENCRYPTED PRIVATE KEY-----\n"
    "MIH0MF8GCSqGSIb3DQEFDTBSMDEGCSqGSIb3DQEFDDAkBBBuHuaoYf4yXWjm2lh4\n"
    "x4Z4AgIIADAMBggqhkiG9w0CCQUAMB0GCWCGSAFlAwQBKgQQfVu494a+BKRB2vCr\n"
    "y+weiASBkGSQSBrNrVscTUB8u4oMRVWg3DlhbMJRgU7MvegoPltOTtyLOSBhWb2U\n"
    "86w0N1Ioq9TfKlhJS58KfXOaiK5tG5YUeFBBT7SVNoIleTLg8JkmJ85Rft+HJUMK\n"
    "65lQ2wGI93NdTHb8G/euls8d4ZoAaR7hjvIqAXn++I76VlHhyA3/CwgoNjiKKzNq\n"
    "LxnHdNhOYA==\n"
    "-----END ENCRYPTED PRIVATE KEY-----\n";
static const char fio___test_tls_pem_garbage[] =
    "-----BEGIN CERTIFICATE-----\n"
    "!!!! not base64 !!!!\n"
    "-----END CERTIFICATE-----\n"
    "-----BEGIN EC PRIVATE KEY-----\n"
    "!!!! not base64 !!!!\n"
    "-----END EC PRIVATE KEY-----\n";
/* A second P-256 key (mismatches the certificate above). */
static const char fio___test_tls_pem_p256b_key[] =
    "-----BEGIN EC PRIVATE KEY-----\n"
    "MHcCAQEEIMdo8Tg+7vKHJXIjFAg9lWBPqsBBhssV/zjwVz+xGIddoAoGCCqGSM49\n"
    "AwEHoUQDQgAEdpsqb+CJPxyZjH/JSVcgfhqlXxtnZ9O5kGpiu8KTGYXziPdXYG2B\n"
    "UkmVx2j2BlR2wrCLiUFN6o6gqF7n8q1xsg==\n"
    "-----END EC PRIVATE KEY-----\n";
/* Self-signed Ed25519 certificate (CN=localhost). */
static const char fio___test_tls_pem_ed_cert[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIBPjCB8aADAgECAhQi63ooyzhcwxJtmbhOvPbmKx2i4TAFBgMrZXAwFDESMBAG\n"
    "A1UEAwwJbG9jYWxob3N0MCAXDTI2MTAyODEzMjUyNloYDzIxMjYxMDA0MTMyNTI2\n"
    "WjAUMRIwEAYDVQQDDAlsb2NhbGhvc3QwKjAFBgMrZXADIQDZi3/DtsEXw53INwiA\n"
    "CKR7lfjrRUepyZHNoyK9MymNQaNTMFEwHQYDVR0OBBYEFHlxcBsuLaOw81LX8tRt\n"
    "L14mYFPYMB8GA1UdIwQYMBaAFHlxcBsuLaOw81LX8tRtL14mYFPYMA8GA1UdEwEB\n"
    "/wQFMAMBAf8wBQYDK2VwA0EAXpUNbSaLwBt/NPlT4TmnO/DU9kkvBnFOrK7ngHDF\n"
    "F2nejG2FdQ5iy53i5LB3lWpNGKUfQB2ixDugAZ6kHVtODQ==\n"
    "-----END CERTIFICATE-----\n";
/* Its private key. */
static const char fio___test_tls_pem_ed_key[] =
    "-----BEGIN PRIVATE KEY-----\n"
    "MC4CAQAwBQYDK2VwBCIEIH2mQsMTGREH2mDMOP3egSfvIsH9gRh5IkuqB8kFwvXk\n"
    "-----END PRIVATE KEY-----\n";
/* Another Ed25519 key (mismatch). */
static const char fio___test_tls_pem_ed_key2[] =
    "-----BEGIN PRIVATE KEY-----\n"
    "MC4CAQAwBQYDK2VwBCIEIFMf/aaaJLWiCygmsBhBV/Wy0BSXvDgA5+bLJUo0Du/m\n"
    "-----END PRIVATE KEY-----\n";
/* Self-signed RSA-2048 certificate (CN=localhost). */
static const char fio___test_tls_pem_rsa_cert[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDCzCCAfOgAwIBAgIUMYFzSavHogoyi/M4c8IjmFyGfo0wDQYJKoZIhvcNAQEL\n"
    "BQAwFDESMBAGA1UEAwwJbG9jYWxob3N0MCAXDTI2MTAyODEzMjUyNloYDzIxMjYx\n"
    "MDA0MTMyNTI2WjAUMRIwEAYDVQQDDAlsb2NhbGhvc3QwggEiMA0GCSqGSIb3DQEB\n"
    "AQUAA4IBDwAwggEKAoIBAQCjYsrRSWIQHg3grEGCUOa7GB3auQpS2yCnYD9Dm+dJ\n"
    "34rHXdR+yjHq9jRNSnzHoKKJ3SPeqZwFsDPqqJ4xyvx9qvj3WX9Plw6ezji93OUD\n"
    "3pjSsxOePVq42ZUgHvNigMWC448XFuIuz6wEH3Va0XZWwEdIoc2caIO1r4MTg1lW\n"
    "AwK1fKCOpvp/YAFe1zhj1s18mBm/c4NTO9iGfsRCK2/+P2KGlH2p+4At4uMPXXmg\n"
    "43nYXB1D75hmZn6exVgq0NjGTSqH9/EBnQwQK85btzimT81W7oJGMSSF0ixLfbPy\n"
    "+MJz+1Vz9nQtN3/aKgfxj5SmL8Ez7KiUnXH3GnH4/Ky5AgMBAAGjUzBRMB0GA1Ud\n"
    "DgQWBBQsZH6jvs1DCVLmv7WPxlc19bJepjAfBgNVHSMEGDAWgBQsZH6jvs1DCVLm\n"
    "v7WPxlc19bJepjAPBgNVHRMBAf8EBTADAQH/MA0GCSqGSIb3DQEBCwUAA4IBAQAr\n"
    "Uw4YrJUfwg+xiDj2Fv0dZ809+GnqQaxUc147E7oBTSDay28QmSCB6+4r6dfA3HC7\n"
    "8+O3JIq+gdMOuzaQYIWpx+VHWqaL7ceaIBK27YZznueqcPCvvjLvv0reATOj1+7K\n"
    "hAxkYzXPFvqVwqlsDFWZf72tbMSrLFK5ZthmCl6TLFXaT1CNGMLr/lkrPHkqrYac\n"
    "LepAJvMc503BW+7rViJDx7al6MakyrOiPua+p30wwbRVFEvz5Xy+T2ueDLl7KSex\n"
    "q2Lol/bJhKCYl4/OlOnqEJUbpnwk3Q/WCHQPmnSoyhxabVA0kNLkqo+OMiWE6QQb\n"
    "mZhx/WHb2x1FmVhRw/12\n"
    "-----END CERTIFICATE-----\n";
/* Its private key. */
static const char fio___test_tls_pem_rsa_key[] =
    "-----BEGIN PRIVATE KEY-----\n"
    "MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQCjYsrRSWIQHg3g\n"
    "rEGCUOa7GB3auQpS2yCnYD9Dm+dJ34rHXdR+yjHq9jRNSnzHoKKJ3SPeqZwFsDPq\n"
    "qJ4xyvx9qvj3WX9Plw6ezji93OUD3pjSsxOePVq42ZUgHvNigMWC448XFuIuz6wE\n"
    "H3Va0XZWwEdIoc2caIO1r4MTg1lWAwK1fKCOpvp/YAFe1zhj1s18mBm/c4NTO9iG\n"
    "fsRCK2/+P2KGlH2p+4At4uMPXXmg43nYXB1D75hmZn6exVgq0NjGTSqH9/EBnQwQ\n"
    "K85btzimT81W7oJGMSSF0ixLfbPy+MJz+1Vz9nQtN3/aKgfxj5SmL8Ez7KiUnXH3\n"
    "GnH4/Ky5AgMBAAECggEAA3orji8n9zyDnt+yv3C0Oi0gqflZFFfnilaa09ukHUcm\n"
    "i/cBXYVuG6aZ+pR2edNjCUqJ5B1dFpbK3rv3Se7jrIweozl0RvU/DdrXRMCBF7Hg\n"
    "+voYeR/IfUe/vb0nVgDJluD71LuigAWnq3AgZyb2Szvpej6UpjcvN04dgF4I70EZ\n"
    "Er/WuHE3+WvfOP+TvtIRFp9HHsP7rtygtt6ISLE8hjcOVuPMeW7PzH9UxNKAZljr\n"
    "w9iF8HAi3/EcN5jSEF8HFvFUbNZtX3F+G+uTLhR6MgjFQi9FjW+s+glWJFYoqlYd\n"
    "uULleFJNvTQU/MGnbnHlegcSNY0kaLnmxsugRv4vJQKBgQDgJC1+YfruxnFjlqm+\n"
    "L2G3syATU2n6KRhz5WZKyWMhWL77+wcQ3sllbpclBP/2sKa2QpOzU8PSAI15LMmQ\n"
    "P849i9nUdk5NpSYCBr/FZtBoN2aHztB7u1mvGStuGaB2itCek4n63Hq+DKnA93ai\n"
    "xEZRQ82bim1oCDwhC8RE0wtrEwKBgQC6m+jVuo+fULVlcCJ90rmqNP93DCsvMkeG\n"
    "0c+uqF30uukFA0ckqPvqyT6Ul9OMnL+hClxObzV3N0BTe94RkIxM7C9MC6IDW144\n"
    "ijQ4Ss0G6ZWlKkqSjogiNeDFRSmXyBuRE3SgF/vP/ru365Obpncu9sSlK6ppgJjL\n"
    "01dTUDHWgwKBgEhIgn9ie7krOAAE44LY5uuGEc8JtICfZipiDYYKwjRL2XOLtfCj\n"
    "2FEkxAuFPELDH1Bh0h4f4mPEF9u/LNWfLhS9GCeja4cwvjwjDw9LI+SoESG0+Zkd\n"
    "h0Onm1DrZMs14ydwryu+lLrbfJcldxXwtDia5CDIGJZXKYUCtW34nZMPAoGBAKc2\n"
    "b5OcoXeLlY+4xF9kxNO8u7GMKOIYtrkbJnMGEXLrTUQdpNE3zsya2cXCqmCUUK5Y\n"
    "H1B1HD6gNrbYBqxMjzuaY+NLhThm1e3qAVLzy/ZGtr4SQFJE/71P9AIwsE1+lR0R\n"
    "hu6xrwW457KtmE3gA2oE2z55+zCgd8okrVpNQOtLAoGAAMwHH6bmAWxJ2d/dDJHx\n"
    "aAfTjOgxumAjw/rPU+pLmHgp9JhpoCUbz8SAxQMp4PmAbytnu8Itppxp6bIsJB/d\n"
    "p89przk6XZOITtSsr55ARhQeDS+kD/IFujHrKnXlDQAw6BL41m/9tCUwJt3jen8g\n"
    "AbvoGQK51/2ch4KqyGy5sgs=\n"
    "-----END PRIVATE KEY-----\n";
/* Another RSA-2048 key (mismatch). */
static const char fio___test_tls_pem_rsa_key2[] =
    "-----BEGIN PRIVATE KEY-----\n"
    "MIIEuwIBADANBgkqhkiG9w0BAQEFAASCBKUwggShAgEAAoIBAQDIfdZ2PSA8xQSG\n"
    "9ZLvpyM6vYUpfTGLHtU85vK626UyFsIndSzBtqAB9y0rok2bwfKqGArjeBN7OXKw\n"
    "k1oNs/d7ntMxR1SRdFNxzsuA23CDIZ+ucVS5Z1RheMxZ9qZrpsvVSxkUt/FpqcZB\n"
    "PGgi5ArwPNXBOjLVCjobtUDWwHUNfHXbjJ/9YSZAhMfIyd4zFJJUSFlPuXBSXfGM\n"
    "gvjIXROoxiPxzXQM84C0sPsHqsZMp0efLE48IReQpWU99reZDXXtoJOk1Ohq1UWA\n"
    "FI/hC451flp1vgzcpPqp0k0QXTMObppeGo296CSwGrwVZEskE2oMedNzPXaQKUuP\n"
    "m7TYbeK1AgMBAAECggEACDC10mI4XysoPnApRr+D7aog1rtNyxJhISoJF++Rncx2\n"
    "tYBwqeAxRKtJjdsFQxvm8yJkzXqSBypnYEXAu8cSkNsB4yU4E/IV15QcVaEBRWYZ\n"
    "/RkVnjZVqN6zsGgmT8cCVC4LcgB6rPeZyP/z/BrnRH2juM9NFM1kZs3efndhwMF2\n"
    "jsLTPWGxKB4Kw66SQ3Ja4Y5113YYCtUmpC0vUgdlje2XZqiOdoBzC6WBIhBeOOhI\n"
    "ow4kWo6zqKSbchcKafJesrbzYG+F/bI0pal+O6GS7YWZ8o1vLmbXYi+oAS+Lqesy\n"
    "tlCa+5DaUOz8QfLPC1I1+rDVuCX9v4Z3F4Z7CgaxUQKBgQDwwdjKLSPYKQ1bQJ0f\n"
    "dBbkm4QW02TuoTUyjsBjKOjHrQoqQEibtbxi9at+pem2YDh5Li5kHgU3AeruBnmo\n"
    "6lp8IkdxuvyqPAOP2ktN78cA1njrYHdFyHIZCGs26RvsC/7oEToAHX/VXnwrsiqn\n"
    "OM7nkAVnW5YWEGAUM+EPUiaR5QKBgQDVL18pxbpNITmFXzF1qpap+iNvjegSlojG\n"
    "g5LewIrIl/bYm9dOYO3dynF8hMyIf6PY/7oCPiLus0cdbyQVBLr+eL3iQ9VJImTD\n"
    "1gNnpyHu1YvInfQRRa4iAR1wxNUxWtnGYR5LrgySwKE/bWDkqmN8ZlYef3pBA7jO\n"
    "Xi8zqexAkQKBgQCSEAjGH7yvV7iStBCfFH8PEoqAjp+dZpAvrAtsHVWcTo7sMzPZ\n"
    "w9gW8ZSpsV6pFpcsbagXdV8UkEVHQwZsREHBLcrKZPcFEIPYskcdcWr6RyP1KQEz\n"
    "hPgz17e0CJi4DE8J2+mkSEFV4YzTtbPK39wnAGlG23miySyBFThlYyGjQQKBgAPN\n"
    "eYQUO9N+YTlIzmgL5+4WQSWPQXF7b+sik68XfiI/265XcUq7neKT5txSvhc9N536\n"
    "ScYad6MMXZtfzSP21qMNYk2z6g0I6QU5PwxNReStuHWWo7DfLlD69hx51lewCCxq\n"
    "tRCihSTk/LtV5yGNDIM+jBC1M2gPee9sbKkeQdGRAn9IztJNfnC1ku5mDv6sSmgx\n"
    "7ILgpNIrEV40PPisXCNHTiWortyYZue5r9XQvpgqgw7Ag31BMpv+TOwp3QW+Jrex\n"
    "/GkURnsKEx5kXAdlEmncJsLiS4SVr11hdVWRemSa+c5BOtlKaUDBLZKFuCdFyMAq\n"
    "NVX8BB3RzMYCledhXOkY\n"
    "-----END PRIVATE KEY-----\n";

/** Writes `data` to `path`; returns 0 on success. */
FIO_SFUNC int fio___test_tls_pem_write(const char *path, const char *data) {
  FILE *f = fopen(path, "wb");
  if (!f)
    return -1;
  size_t len = strlen(data);
  int r = (fwrite(data, 1, len, f) == len) ? 0 : -1;
  fclose(f);
  return r;
}

/** Writes a fixture to `tmp/fio-tls-<tag>-<pid>.pem`, path returned in buf. */
FIO_SFUNC const char *fio___test_tls_pem_file(char *buf,
                                              size_t len,
                                              const char *tag,
                                              const char *data) {
  snprintf(buf, len, "tmp/fio-tls-%s-%d.pem", tag, (int)getpid());
  FIO_ASSERT(!fio___test_tls_pem_write(buf, data),
             "failed to write TLS test PEM file %s (tmp/ folder missing?)",
             buf);
  return buf;
}

/** Writes the `count` PEM blocks of `pems`, in order, to a file named as
 * above (a certificate chain file: leaf first). */
FIO_SFUNC const char *fio___test_tls_pem_join(char *buf,
                                              size_t len,
                                              const char *tag,
                                              const char **pems,
                                              size_t count) {
  snprintf(buf, len, "tmp/fio-tls-%s-%d.pem", tag, (int)getpid());
  FILE *f = fopen(buf, "wb");
  int ok = !!f;
  for (size_t i = 0; ok && i < count; ++i)
    ok = fwrite(pems[i], 1, strlen(pems[i]), f) == strlen(pems[i]);
  if (f)
    ok &= !fclose(f);
  FIO_ASSERT(ok,
             "failed to write TLS test PEM file %s (tmp/ folder missing?)",
             buf);
  return buf;
}

/** Writes `leaf` followed by `ca` (a certificate chain file); see above. */
FIO_SFUNC const char *fio___test_tls_pem_chain(char *buf,
                                               size_t len,
                                               const char *tag,
                                               const char *leaf,
                                               const char *ca) {
  const char *pems[2] = {leaf, ca};
  return fio___test_tls_pem_join(buf, len, tag, pems, 2);
}

/** Builds a server (or client) context from `tls`, freeing both; returns 1 if
 * built. */
FIO_SFUNC int fio___test_tls_pem_builds2(fio_io_functions_s fn,
                                         fio_io_tls_s *tls,
                                         uint8_t is_client) {
  int log_level = FIO_LOG_LEVEL_GET();
  FIO_LOG_LEVEL_SET(FIO_LOG_LEVEL_NONE); /* expected load errors */
  void *ctx = fn.build_context(tls, is_client);
  FIO_LOG_LEVEL_SET(log_level);
  fio_io_tls_free(tls);
  if (!ctx)
    return 0;
  fn.free_context(ctx);
  return 1;
}

/** Builds a server context from `tls`, freeing both; returns 1 if built. */
FIO_SFUNC int fio___test_tls_pem_builds(fio_io_functions_s fn,
                                        fio_io_tls_s *tls) {
  return fio___test_tls_pem_builds2(fn, tls, 0);
}

FIO_SFUNC void fio___test_tls_pem_load(fio_io_functions_s fn,
                                       const char *name,
                                       int pk_password_supported) {
  char cert[64], key[64], enc[64], bad[64], p256b[64];
  char edc[64], edk[64], edk2[64], rsac[64], rsak[64], rsak2[64];
  char cc[64], ck[64], ck8[64], ckbad[64];
  char pc[64], pk[64], pk8[64], pkbad[64], puc[64], puk[64], puk8[64];
  char mrc[64], mrk[64], mpc[64], mpk[64];
  const char *missing = "tmp/fio-tls-missing-file.pem";
  fio___test_tls_pem_file(cert, sizeof(cert), "cert", fio___test_tls_pem_cert);
  fio___test_tls_pem_file(key, sizeof(key), "key", fio___test_tls_pem_key);
  fio___test_tls_pem_file(enc, sizeof(enc), "enc", fio___test_tls_pem_key_enc);
  fio___test_tls_pem_file(bad, sizeof(bad), "bad", fio___test_tls_pem_garbage);
  fio___test_tls_pem_file(p256b, 64, "p256b", fio___test_tls_pem_p256b_key);
  fio___test_tls_pem_file(edc, 64, "edc", fio___test_tls_pem_ed_cert);
  fio___test_tls_pem_file(edk, 64, "edk", fio___test_tls_pem_ed_key);
  fio___test_tls_pem_file(edk2, 64, "edk2", fio___test_tls_pem_ed_key2);
  fio___test_tls_pem_file(rsac, 64, "rsac", fio___test_tls_pem_rsa_cert);
  fio___test_tls_pem_file(rsak, 64, "rsak", fio___test_tls_pem_rsa_key);
  fio___test_tls_pem_file(rsak2, 64, "rsak2", fio___test_tls_pem_rsa_key2);
  /* compressed P-256 point (SEC 1 03 || x): leaf + CA chain, its key */
  fio___test_tls_pem_chain(cc,
                           64,
                           "cc",
                           fio___test_ecc_p256_leaf,
                           fio___test_ecc_p256_ca);
  fio___test_tls_pem_file(ck, 64, "ck", fio___test_ecc_p256_key);
  fio___test_tls_pem_file(ck8, 64, "ck8", fio___test_ecc_p256_key_pkcs8);
  fio___test_tls_pem_file(ckbad, 64, "ckbad", fio___test_ecc_p256_key_badpub);
  /* P-384: compressed (02/03) and uncompressed (04) chains and keys */
  fio___test_tls_pem_chain(pc,
                           64,
                           "pc",
                           fio___test_ecc_p384_leaf,
                           fio___test_ecc_p384_ca);
  fio___test_tls_pem_file(pk, 64, "pk", fio___test_ecc_p384_key);
  fio___test_tls_pem_file(pk8, 64, "pk8", fio___test_ecc_p384_key_pkcs8);
  fio___test_tls_pem_file(pkbad, 64, "pkbad", fio___test_ecc_p384_key_badpub);
  fio___test_tls_pem_chain(puc,
                           64,
                           "puc",
                           fio___test_ecc_p384u_leaf,
                           fio___test_ecc_p384u_ca);
  fio___test_tls_pem_file(puk, 64, "puk", fio___test_ecc_p384u_key);
  fio___test_tls_pem_file(puk8, 64, "puk8", fio___test_ecc_p384u_key_pkcs8);
  /* leaf + intermediate chains (RSA and P-384 leaves) */
  fio___test_tls_pem_chain(mrc,
                           64,
                           "mrc",
                           fio___test_mtls_rsa_leaf,
                           fio___test_mtls_int);
  fio___test_tls_pem_file(mrk, 64, "mrk", fio___test_mtls_rsa_key);
  fio___test_tls_pem_chain(mpc,
                           64,
                           "mpc",
                           fio___test_mtls_p384_leaf,
                           fio___test_mtls_int);
  fio___test_tls_pem_file(mpk, 64, "mpk", fio___test_mtls_p384_key);
  unlink(missing);
  struct {
    const char *cert;
    const char *key;
    const char *pass;
    int builds;
    const char *desc;
  } cases[] = {
      {cert, key, NULL, 1, "valid certificate and key"},
      {missing, key, NULL, 0, "missing certificate file"},
      {cert, missing, NULL, 0, "missing key file"},
      {bad, key, NULL, 0, "garbage certificate PEM"},
      {cert, bad, NULL, 0, "garbage key PEM"},
      {cert, enc, "wrong", 0, "wrong key password"},
      {cert, enc, NULL, 0, "encrypted key without a password"},
      {cert, enc, "secret", pk_password_supported, "correct key password"},
      /* the key must match the leaf (X509_check_private_key parity) */
      {edc, edk, NULL, 1, "valid Ed25519 certificate and key"},
      {rsac, rsak, NULL, 1, "valid RSA certificate and key"},
      {cert, p256b, NULL, 0, "mismatched P-256 key"},
      {edc, edk2, NULL, 0, "mismatched Ed25519 key"},
      {rsac, rsak2, NULL, 0, "mismatched RSA key"},
      {cert, edk, NULL, 0, "Ed25519 key for a P-256 certificate"},
      {rsac, key, NULL, 0, "P-256 key for an RSA certificate"},
      {edc, rsak, NULL, 0, "RSA key for an Ed25519 certificate"},
      /* compressed EC points in the certificate and / or the key file */
      {cc, ck, NULL, 1, "compressed P-256 certificate and SEC1 key"},
      {cc, ck8, NULL, 1, "compressed P-256 certificate and PKCS#8 key"},
      {cc, p256b, NULL, 0, "mismatched key, compressed P-256 certificate"},
      {cc, key, NULL, 0, "other P-256 key, compressed P-256 certificate"},
      {cert, ck, NULL, 0, "compressed P-256 key for another certificate"},
      {cc, ckbad, NULL, 0, "P-256 key embedding another public key"},
      /* P-384 keys (SEC1 / PKCS#8, compressed / uncompressed points) */
      {pc, pk, NULL, 1, "compressed P-384 certificate and SEC1 key"},
      {pc, pk8, NULL, 1, "compressed P-384 certificate and PKCS#8 key"},
      {puc, puk, NULL, 1, "uncompressed P-384 certificate and SEC1 key"},
      {puc, puk8, NULL, 1, "uncompressed P-384 certificate and PKCS#8 key"},
      {mpc, mpk, NULL, 1, "P-384 leaf + intermediate chain and key"},
      {mrc, mrk, NULL, 1, "RSA leaf + intermediate chain and key"},
      {pc, puk, NULL, 0, "other P-384 key, compressed P-384 certificate"},
      {puc, pk, NULL, 0, "other P-384 key, uncompressed P-384 certificate"},
      {puc, pk8, NULL, 0, "other P-384 PKCS#8 key, P-384 certificate"},
      {mpc, puk, NULL, 0, "other P-384 key, P-384 leaf + intermediate"},
      {pc, pkbad, NULL, 0, "P-384 key embedding another public key"},
      {pc, ck, NULL, 0, "P-256 key for a P-384 certificate"},
      {cc, pk, NULL, 0, "P-384 key for a P-256 certificate"},
      {cert, puk, NULL, 0, "P-384 key for an uncompressed P-256 certificate"},
      {rsac, puk, NULL, 0, "P-384 key for an RSA certificate"},
      {pc, edk, NULL, 0, "Ed25519 key for a P-384 certificate"},
      {mrc, mpk, NULL, 0, "P-384 key for an RSA leaf"},
  };
  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    fio_io_tls_s *tls = fio_io_tls_new();
    fio_io_tls_cert_add(tls,
                        "localhost",
                        cases[i].cert,
                        cases[i].key,
                        cases[i].pass);
    FIO_ASSERT(fio___test_tls_pem_builds(fn, tls) == cases[i].builds,
               "%s: %s should %s the context build",
               name,
               cases[i].desc,
               cases[i].builds ? "pass" : "fail");
    /* several certificates (SNI path): one bad entry fails the whole build */
    tls = fio_io_tls_new();
    fio_io_tls_cert_add(tls, "a.example.com", NULL, NULL, NULL);
    fio_io_tls_cert_add(tls,
                        "localhost",
                        cases[i].cert,
                        cases[i].key,
                        cases[i].pass);
    fio_io_tls_cert_add(tls, "b.example.com", cert, key, NULL);
    FIO_ASSERT(fio___test_tls_pem_builds(fn, tls) == cases[i].builds,
               "%s: %s (SNI, several certificates) should %s the build",
               name,
               cases[i].desc,
               cases[i].builds ? "pass" : "fail");
    /* client certificate (mTLS): loaded and checked the same way */
    tls = fio_io_tls_new();
    fio_io_tls_cert_add(tls,
                        "localhost",
                        cases[i].cert,
                        cases[i].key,
                        cases[i].pass);
    FIO_ASSERT(fio___test_tls_pem_builds2(fn, tls, 1) == cases[i].builds,
               "%s: %s (client certificate) should %s the client build",
               name,
               cases[i].desc,
               cases[i].builds ? "pass" : "fail");
  }
  { /* names without files are still self-signed (single / several / none) */
    fio_io_tls_s *tls = fio_io_tls_new();
    fio_io_tls_cert_add(tls, "localhost", NULL, NULL, NULL);
    FIO_ASSERT(fio___test_tls_pem_builds(fn, tls),
               "%s: a name without files should get a self-signed cert",
               name);
    tls = fio_io_tls_new();
    fio_io_tls_cert_add(tls, "a.example.com", NULL, NULL, NULL);
    fio_io_tls_cert_add(tls, "b.example.com", NULL, NULL, NULL);
    FIO_ASSERT(fio___test_tls_pem_builds(fn, tls),
               "%s: names without files should get self-signed certs",
               name);
    FIO_ASSERT(fio___test_tls_pem_builds(fn, fio_io_tls_new()),
               "%s: a server without certificates should be self-signed",
               name);
  }
  unlink(cert);
  unlink(key);
  unlink(enc);
  unlink(bad);
  unlink(p256b);
  unlink(edc);
  unlink(edk);
  unlink(edk2);
  unlink(rsac);
  unlink(rsak);
  unlink(rsak2);
  unlink(cc);
  unlink(ck);
  unlink(ck8);
  unlink(ckbad);
  unlink(pc);
  unlink(pk);
  unlink(pk8);
  unlink(pkbad);
  unlink(puc);
  unlink(puk);
  unlink(puk8);
  unlink(mrc);
  unlink(mrk);
  unlink(mpc);
  unlink(mpk);
  fprintf(stderr, "* %s PEM load failures fail closed: OK\n", name);
}

/* *****************************************************************************
TLS requested => TLS is a must: fio_io_listen / fio_io_connect fail when the
TLS context can't be built (no reactor needed: the error is synchronous).
***************************************************************************** */
static size_t fio___test_tls_must_stopped;
static size_t fio___test_tls_must_failed;
FIO_SFUNC void fio___test_tls_must_on_stop(fio_io_protocol_s *p, void *u) {
  ++fio___test_tls_must_stopped;
  (void)p, (void)u;
}
FIO_SFUNC void fio___test_tls_must_on_failed(fio_io_protocol_s *p, void *u) {
  FIO_ASSERT(u == (void *)&fio___test_tls_must_failed, "on_failed udata");
  ++fio___test_tls_must_failed;
  (void)p;
}

FIO_SFUNC void fio___test_tls_must(fio_io_functions_s fn, const char *name) {
  static fio_io_protocol_s pr;
  char cert[64], key[64], p256b[64];
  const char *missing = "tmp/fio-tls-missing-file.pem";
  fio___test_tls_pem_file(cert, sizeof(cert), "mcert", fio___test_tls_pem_cert);
  fio___test_tls_pem_file(key, sizeof(key), "mkey", fio___test_tls_pem_key);
  fio___test_tls_pem_file(p256b, 64, "mp256b", fio___test_tls_pem_p256b_key);
  unlink(missing);
  pr = (fio_io_protocol_s){.io_functions = fn};
  int log_level = FIO_LOG_LEVEL_GET();
  FIO_LOG_LEVEL_SET(FIO_LOG_LEVEL_FATAL); /* expected build errors */
  struct {
    const char *cert, *key, *desc;
  } bad[] = {{missing, key, "missing certificate file"},
             {cert, p256b, "mismatched private key"}};
  for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); ++i) {
    fio___test_tls_must_stopped = 0;
    fio_io_tls_s *tls = fio_io_tls_new();
    fio_io_tls_cert_add(tls, "localhost", bad[i].cert, bad[i].key, NULL);
    fio_io_listener_s *l = fio_io_listen(.url = "tcp://127.0.0.1:0",
                                         .protocol = &pr,
                                         .tls = tls,
                                         .on_stop = fio___test_tls_must_on_stop,
                                         .hide_from_log = 1); /* owns tls */
    FIO_ASSERT(!l, "%s: listen with a %s must fail", name, bad[i].desc);
    FIO_ASSERT(fio___test_tls_must_stopped == 1,
               "%s: failed TLS listen should call on_stop once",
               name);
  }
  { /* URL-derived TLS object (freed by fio_io_listen on failure) */
    char url[160];
    fio___test_tls_must_stopped = 0;
    snprintf(url,
             sizeof(url),
             "tcp://127.0.0.1:0/?cert=%s&key=%s",
             missing,
             key);
    fio_io_listener_s *l = fio_io_listen(.url = url,
                                         .protocol = &pr,
                                         .on_stop = fio___test_tls_must_on_stop,
                                         .hide_from_log = 1);
    FIO_ASSERT(!l, "%s: listen with URL TLS hints (missing cert)", name);
    FIO_ASSERT(fio___test_tls_must_stopped == 1, "%s: URL on_stop", name);
  }
  { /* client: a missing trust file fails the build; on_failed runs once */
    fio___test_tls_must_failed = 0;
    fio_io_tls_s *tls = fio_io_tls_new();
    fio_io_tls_trust_add(tls, missing);
    fio_io_s *io = fio_io_connect("tls://localhost:9",
                                  .protocol = &pr,
                                  .on_failed = fio___test_tls_must_on_failed,
                                  .udata = (void *)&fio___test_tls_must_failed,
                                  .tls = tls, /* owned by the call */
                                  .timeout = 100);
    FIO_ASSERT(!io, "%s: connect with a bad TLS object must fail", name);
    fio_queue_perform_all(fio_io_queue());
    fio_queue_perform_all(fio_io_queue());
    FIO_ASSERT(fio___test_tls_must_failed == 1,
               "%s: on_failed should run exactly once (%zu)",
               name,
               fio___test_tls_must_failed);
  }
  FIO_LOG_LEVEL_SET(log_level);
  unlink(cert);
  unlink(key);
  unlink(p256b);
  fprintf(stderr, "* %s listen / connect fail when TLS can't build: OK\n", name);
}
#endif /* H___FIO_TEST_TLS_PEM_LOAD_H */
