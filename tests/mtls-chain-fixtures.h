/* *****************************************************************************
Shared test fixtures - a client certificate chain with an intermediate CA

    root (P-384, self-signed, SHA-384, CN=mTLS Test Root CA)
      -> intermediate (P-256, SHA-384, CA:TRUE pathlen:0)
           -> RSA-2048 leaf (SHA-256)
           -> P-384 leaf (SHA-256)
      -> big intermediate (RSA-4096, SHA-384, pathlen:0, ~9.9 KB padded)
           -> big RSA-4096 leaf (SHA-256, ~10.4 KB padded)

Peers trust only the root, so a client must send the intermediate too. Valid
from 2025-01-01 until 2125-12-31; CN=localhost leaves carry SAN
DNS:localhost, CA:FALSE, EKU serverAuth + clientAuth. Serials 0x0A01-0x0A06.
Test-only keys. Generated with OpenSSL 3 (`openssl x509 -req ...`, RSA key in
PKCS#1 form via `openssl genrsa -traditional`, EC keys in SEC 1 form).
***************************************************************************** */
#ifndef H___FIO_TEST_MTLS_CHAIN_FIXTURES_H
#define H___FIO_TEST_MTLS_CHAIN_FIXTURES_H

/* The root CA (P-384), the only certificate peers trust. */
static const char fio___test_mtls_root[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIBqTCCAS+gAwIBAgICCgEwCgYIKoZIzj0EAwMwHDEaMBgGA1UEAwwRbVRMUyBU\n"
    "ZXN0IFJvb3QgQ0EwIBcNMjUwMTAxMDAwMDAwWhgPMjEyNTEyMzEyMzU5NTlaMBwx\n"
    "GjAYBgNVBAMMEW1UTFMgVGVzdCBSb290IENBMHYwEAYHKoZIzj0CAQYFK4EEACID\n"
    "YgAE6Au1DVPPhaNepRzYJv91rmbIahAy7dh84n+SDK/kF9nSciwp1RoXplB26x6G\n"
    "QfoGAuL4QssN6aHFJNRTO4eyEAVxPdnjo8cfY4TwiRr9I7N4B/NdaGHUgAByRYxd\n"
    "A+3oo0IwQDAPBgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBBjAdBgNVHQ4E\n"
    "FgQUmFrdQH0yVJapQRTFuPNm8lhz6AMwCgYIKoZIzj0EAwMDaAAwZQIxAOep7/+P\n"
    "z34jNmA7Ipp7LEZSPqnJAS7oSvvK8Dh7BYYq1/d8fbpRmxfwhV57EgbdzwIwd+7y\n"
    "kFJy/kStCqkpqo5FC0bME15Txo0KchN+OyU6GEFsyJsTGNGvljINZfbLKkni\n"
    "-----END CERTIFICATE-----\n";

/* The intermediate CA (P-256), issued by the root. */
static const char fio___test_mtls_int[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIBuDCCAT6gAwIBAgICCgIwCgYIKoZIzj0EAwMwHDEaMBgGA1UEAwwRbVRMUyBU\n"
    "ZXN0IFJvb3QgQ0EwIBcNMjUwMTAxMDAwMDAwWhgPMjEyNTEyMzEyMzU5NTlaMCQx\n"
    "IjAgBgNVBAMMGW1UTFMgVGVzdCBJbnRlcm1lZGlhdGUgQ0EwWTATBgcqhkjOPQIB\n"
    "BggqhkjOPQMBBwNCAATmVQXZkSe5TeaQ+JufRCK42bRaFpqRgcu8FcG2iQAL6/AF\n"
    "Ii5j8p5zj328BT7NsRx0mmwIkSTKf00fRUBia5mio2YwZDASBgNVHRMBAf8ECDAG\n"
    "AQH/AgEAMA4GA1UdDwEB/wQEAwIBBjAdBgNVHQ4EFgQUiWEuOoT5Mh76AJ8uo/1i\n"
    "z1Hm1cQwHwYDVR0jBBgwFoAUmFrdQH0yVJapQRTFuPNm8lhz6AMwCgYIKoZIzj0E\n"
    "AwMDaAAwZQIwZCJzxXTbdAXA+AMDEuRMtu67MhP8sZaKwuilHEaHIHW/lrHJ0mlL\n"
    "xSRQaiy5G9PQAjEA7uEQ/2aXayTAjBa1ZnUnWUAsq7srmP/5ujB0gBp+muBgAmhK\n"
    "gEgjetI9zZH0QEYD\n"
    "-----END CERTIFICATE-----\n";

/* RSA-2048 leaf, issued by the intermediate. */
static const char fio___test_mtls_rsa_leaf[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIICjDCCAjKgAwIBAgICCgMwCgYIKoZIzj0EAwIwJDEiMCAGA1UEAwwZbVRMUyBU\n"
    "ZXN0IEludGVybWVkaWF0ZSBDQTAgFw0yNTAxMDEwMDAwMDBaGA8yMTI1MTIzMTIz\n"
    "NTk1OVowFDESMBAGA1UEAwwJbG9jYWxob3N0MIIBIjANBgkqhkiG9w0BAQEFAAOC\n"
    "AQ8AMIIBCgKCAQEAy8BzDanWgVDL/oxIYWA70iNVzvXAXBOsmBdHkdVsphAwyPBd\n"
    "J9HuWQf+UmldF7qH58vlkuqP+oEkt72WGKky3YcI2e0VSdLUMvmrxWzOjYRT50BM\n"
    "UcnBWzpT3jl3j/p38rgQFV3c49fqfybX4BGtScbjjqkuUljD4pfrYHDWX+1GTjn8\n"
    "dFI7ZtamJLlrAw5kvYVdQX5KsKSncBBYaYVxWa7PFX2y/poTH6D/y/GMCsgcHezP\n"
    "RhUs5FmFKRh0F/zXBTdNgA+eEyankwVeBHl1T/Hl5fo3yjj63LYJLDPzDVblm0vU\n"
    "snXfJdtCRKOX3qTeEGsFUS9LnbQMoAHFwKY6CwIDAQABo4GWMIGTMAwGA1UdEwEB\n"
    "/wQCMAAwDgYDVR0PAQH/BAQDAgWgMB0GA1UdJQQWMBQGCCsGAQUFBwMBBggrBgEF\n"
    "BQcDAjAUBgNVHREEDTALgglsb2NhbGhvc3QwHQYDVR0OBBYEFI8/qzgcCjq5azN3\n"
    "E2mqh+499mugMB8GA1UdIwQYMBaAFIlhLjqE+TIe+gCfLqP9Ys9R5tXEMAoGCCqG\n"
    "SM49BAMCA0gAMEUCIAp9a4o3Az/oHAWO+MV9KD8aNqvAwOumRXfseA7pTwisAiEA\n"
    "t/GTrvHOdmey7USPgC6uIr7DfU1qrr/ctDNvVXXwmAQ=\n"
    "-----END CERTIFICATE-----\n";

/* The RSA leaf key (PKCS#1). */
static const char fio___test_mtls_rsa_key[] =
    "-----BEGIN RSA PRIVATE KEY-----\n"
    "MIIEpAIBAAKCAQEAy8BzDanWgVDL/oxIYWA70iNVzvXAXBOsmBdHkdVsphAwyPBd\n"
    "J9HuWQf+UmldF7qH58vlkuqP+oEkt72WGKky3YcI2e0VSdLUMvmrxWzOjYRT50BM\n"
    "UcnBWzpT3jl3j/p38rgQFV3c49fqfybX4BGtScbjjqkuUljD4pfrYHDWX+1GTjn8\n"
    "dFI7ZtamJLlrAw5kvYVdQX5KsKSncBBYaYVxWa7PFX2y/poTH6D/y/GMCsgcHezP\n"
    "RhUs5FmFKRh0F/zXBTdNgA+eEyankwVeBHl1T/Hl5fo3yjj63LYJLDPzDVblm0vU\n"
    "snXfJdtCRKOX3qTeEGsFUS9LnbQMoAHFwKY6CwIDAQABAoIBAEftYtBWfe59tt3f\n"
    "6OdTQRrwNWkPsT3wBhRU14Oga6/99DLjQ4d8oR8V5s5x+paXV0XZYWxpWLVupKcm\n"
    "itee12GDu3ivLL44aoeqQty5iftEVRhERwzyY8tQPm4rLE2L9o8dMjRQw9s9DD+h\n"
    "10B3DQ0H5KomiD8yrcgOqF2Nx+fpqdCS6nhbl0Tr46fjKJJJ3hzL94tfUyhY5dSk\n"
    "gDnGIYktSiVnxWD5YI9LD3V/gwLYswfRoraj+OEgUQTLJTomvM18YHUSAoDJgjtf\n"
    "+ggJoZBiThibdpQH1zEt8vsiXKWI5Sb0cBCjOnbQv3xfD0Q7rRVUx0EIsIErwXHZ\n"
    "5GGTG9ECgYEA+5CvQDQQHlFb50vmdLGxMrx5HW5uetJD9vAodIXtldVPy3ILa2Ug\n"
    "Mma5qWo4dVowIiMR33p+HT8qKeb+zYZWOTtF7WfXWyY4swTpovGwL/kYt/0//sAk\n"
    "0GnF2g9QdFsW/rgQO5gbTweSGXOqgKdZWBPs6fDMQxfuZwHWmRhTXfECgYEAz1f7\n"
    "iLNWDUFDTV6r3RQeW10520J+6vRY3UZS+ylnyWCy38eFIE3add/XR2pTmRymJDAm\n"
    "LhWCaFlFCvIHvzJoObktUmyQohmVS3fnUj23srgTGFU0dWI99O/t/zMMbXxF28TK\n"
    "k9GGTBa0KDNrhTky6SiSkjgmEaLmAB0gp4PrS7sCgYEAwlgma81M3n8wJuImKeTU\n"
    "5kNgtfIIe+5Q/0bNmieVQnnG6X8zf0Zit01RBaa/8zk3kkfRmPqCz4lqw3BnTNQJ\n"
    "bLFz9lKQtyQyDWswK+fXm1gvVoTZma9LQY9VgH7wQtPUUfhU4hVCk61fmzYWdMnU\n"
    "Zg0OrDSROQwV65Ip/aAjV6ECgYBLW5PM5DkTSjP2y8C08hnyZNihSvKr3DDsAyIn\n"
    "rK3O7/kzAeXtjr4mdztb9lX54QK1RlaqCyFBYaiJSNvt7pkY2aeSPGj4sUmkiyik\n"
    "qE+6gdJW1qjhPNp9yjH6vYf3xuHw9P4XaWBhCSNjNQUP/3D0cs2KnSOg506mne9/\n"
    "Up/S/wKBgQD0U0TOE4Z2ihwURJGMt4rb6WrQ1i/l5tXI3XWoGTXb0gFs9ZkFAADt\n"
    "3WgxKNJmpWOrUoqfaFJvD+bR8m28Syfuv4kvSHez8fdf2duOClZrZxKSE6h1LzUq\n"
    "TuB/od5TtEIoiL7K7xskanMlBrFg/TDGtsNx3XR7xM1hF/66eYKsFw==\n"
    "-----END RSA PRIVATE KEY-----\n";

/* P-384 leaf, issued by the intermediate. */
static const char fio___test_mtls_p384_leaf[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIB3jCCAYSgAwIBAgICCgQwCgYIKoZIzj0EAwIwJDEiMCAGA1UEAwwZbVRMUyBU\n"
    "ZXN0IEludGVybWVkaWF0ZSBDQTAgFw0yNTAxMDEwMDAwMDBaGA8yMTI1MTIzMTIz\n"
    "NTk1OVowFDESMBAGA1UEAwwJbG9jYWxob3N0MHYwEAYHKoZIzj0CAQYFK4EEACID\n"
    "YgAEUv94lpRitmNMqXsh+QIznoBnJ2iJyl9hyde7tF+pRWSFjfhPlTZg3sTXY3NE\n"
    "uHrEGEPWw4+w2iNAqBy307FMOg8kDAVpIEUarWfrR0oGXFmgDekIz821D5ojgSSK\n"
    "yeiDo4GWMIGTMAwGA1UdEwEB/wQCMAAwDgYDVR0PAQH/BAQDAgWgMB0GA1UdJQQW\n"
    "MBQGCCsGAQUFBwMBBggrBgEFBQcDAjAUBgNVHREEDTALgglsb2NhbGhvc3QwHQYD\n"
    "VR0OBBYEFNO1FVdG5WBav9x/aTBglZVVXFdIMB8GA1UdIwQYMBaAFIlhLjqE+TIe\n"
    "+gCfLqP9Ys9R5tXEMAoGCCqGSM49BAMCA0gAMEUCIQCjLs/xE6he/FqborFNebHf\n"
    "Fc4q6HYFNvN+RKwYHHIl+gIgJIQrr8KnTMa3CEnEmfhR/51G4z7F6aca0JCGfLsl\n"
    "52M=\n"
    "-----END CERTIFICATE-----\n";

/* The P-384 leaf key (SEC 1). */
static const char fio___test_mtls_p384_key[] =
    "-----BEGIN EC PRIVATE KEY-----\n"
    "MIGkAgEBBDCdRd52oBW1+i7yQh6wHXF6ZneUxtFljMwvt90opwBqJ/iTJ6MYfsF2\n"
    "nP1096iyo9GgBwYFK4EEACKhZANiAARS/3iWlGK2Y0ypeyH5AjOegGcnaInKX2HJ\n"
    "17u0X6lFZIWN+E+VNmDexNdjc0S4esQYQ9bDj7DaI0CoHLfTsUw6DyQMBWkgRRqt\n"
    "Z+tHSgZcWaAN6QjPzbUPmiOBJIrJ6IM=\n"
    "-----END EC PRIVATE KEY-----\n";

/* Big chain (> 16 KiB, exercises multi-record handshake flights): the
 * mTLS root -> RSA-4096 intermediate (9000 byte private extension,
 * pathlen 0) -> RSA-4096 leaf (9000 byte private extension, CN / SAN
 * localhost, serverAuth + clientAuth). Generated by OpenSSL 3. */
/** Big intermediate CA (RSA-4096, padded), signed by the mTLS root. */
static const char *const fio___test_mtls_big_int_lines[] = {
    "-----BEGIN CERTIFICATE-----\n",
    "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\n",
    "-----END CERTIFICATE-----\n",
    NULL};
/* joined by fio___test_mtls_big_init */
static char fio___test_mtls_big_int[16384];

/** Big leaf (RSA-4096, padded), signed by the big intermediate. */
static const char *const fio___test_mtls_big_leaf_lines[] = {
    "-----BEGIN CERTIFICATE-----\n",
    "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\n",
    "tNlF0IfHW5L9FSkjJlvD4hrm+prVrw==\n",
    "-----END CERTIFICATE-----\n",
    NULL};
/* joined by fio___test_mtls_big_init */
static char fio___test_mtls_big_leaf[16384];

/** Big leaf key (RSA-4096, PKCS#1). */
static const char fio___test_mtls_big_key[] =
    "-----BEGIN RSA PRIVATE KEY-----\n"
    "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\n"
    "-----END RSA PRIVATE KEY-----\n";

/** Joins the big certificates' PEM lines (call before using them). */
FIO_SFUNC void fio___test_mtls_big_init(void) {
  struct {
    char *dst;
    const char *const *lines;
  } t[] = {{fio___test_mtls_big_int, fio___test_mtls_big_int_lines},
           {fio___test_mtls_big_leaf, fio___test_mtls_big_leaf_lines}};
  for (size_t i = 0; i < 2; ++i) {
    size_t len = 0;
    for (const char *const *l = t[i].lines; *l; ++l) {
      size_t n = strlen(*l);
      FIO_ASSERT(len + n < sizeof(fio___test_mtls_big_int),
                 "big fixture overflow");
      memcpy(t[i].dst + len, *l, n);
      len += n;
    }
    t[i].dst[len] = 0;
  }
}

#endif /* H___FIO_TEST_MTLS_CHAIN_FIXTURES_H */
