/* *****************************************************************************
Shared test fixtures - compressed EC points (SEC 1 `02/03 || x`)

Certificates and keys whose SubjectPublicKeyInfo / embedded public key use the
compressed point form (RFC 5480 allows it; OpenSSL accepts it). Valid from
2025-01-01 until 2125-12-31; CN=localhost leaves carry SAN DNS:localhost,
CA:FALSE, digitalSignature, EKU serverAuth + clientAuth. Test-only keys.

Generated with OpenSSL 3, per curve (prime256v1 / SHA-256, secp384r1 /
SHA-384), retrying key generation for the documented prefix:

    openssl ecparam -name <curve> -genkey -noout |
        openssl ec -conv_form compressed -out ca.key
    openssl req -new -x509 -key ca.key -<md> -not_before 20250101000000Z \
        -not_after 21251231235959Z -subj "/CN=Compressed P-<n> Test CA" ...
    openssl x509 -req -in leaf.csr -CA ca.pem -CAkey ca.key ... -extfile ...
    openssl pkcs8 -topk8 -nocrypt -in leaf.key   # PKCS#8 form

The "badpub" keys are the leaf keys with their embedded public key replaced
by their CA's (a key file that disagrees with itself).

The `p384u` chain (CA, leaf, SEC 1 and PKCS#8 key) is the uncompressed (04)
counterpart, used by the P-384 signing tests (both point forms). Generated
the same way without `-conv_form compressed`, serials 0x0385 / 0x1385.
***************************************************************************** */
#ifndef H___FIO_TEST_EC_COMPRESSED_FIXTURES_H
#define H___FIO_TEST_EC_COMPRESSED_FIXTURES_H

/* P-256 CA, self-signed, compressed point (prefix 02). */
static const char fio___test_ecc_p256_ca[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIBWjCCAQCgAwIBAgICAlYwCgYIKoZIzj0EAwIwIzEhMB8GA1UEAwwYQ29tcHJl\n"
    "c3NlZCBQLTI1NiBUZXN0IENBMCAXDTI1MDEwMTAwMDAwMFoYDzIxMjUxMjMxMjM1\n"
    "OTU5WjAjMSEwHwYDVQQDDBhDb21wcmVzc2VkIFAtMjU2IFRlc3QgQ0EwOTATBgcq\n"
    "hkjOPQIBBggqhkjOPQMBBwMiAAKaDPCwhFr7iYOe99exAMtOp9f0ruDa7lb+SccV\n"
    "7mPUTqNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMCAQYwHQYDVR0O\n"
    "BBYEFFiXObOyJpPZYn6lKRiJyZkHlpzcMAoGCCqGSM49BAMCA0gAMEUCIQCM9jcE\n"
    "PkCmIOeb4ZEdnrk6Donb4bEiS5lWV49t8NE76QIgZ3/i2QFrBexW3Zgry7blbB6a\n"
    "qqfDxZuzEmXNNMklZgM=\n"
    "-----END CERTIFICATE-----\n";

/* P-256 leaf (CN=localhost) issued by the P-256 CA, compressed (prefix 03). */
static const char fio___test_ecc_p256_leaf[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIBoDCCAUagAwIBAgICElYwCgYIKoZIzj0EAwIwIzEhMB8GA1UEAwwYQ29tcHJl\n"
    "c3NlZCBQLTI1NiBUZXN0IENBMCAXDTI1MDEwMTAwMDAwMFoYDzIxMjUxMjMxMjM1\n"
    "OTU5WjAUMRIwEAYDVQQDDAlsb2NhbGhvc3QwOTATBgcqhkjOPQIBBggqhkjOPQMB\n"
    "BwMiAAMqCQb+EpZAPtFsecqWFFzf+/T3RzicPI9ABimFEACae6OBljCBkzAMBgNV\n"
    "HRMBAf8EAjAAMA4GA1UdDwEB/wQEAwIHgDAdBgNVHSUEFjAUBggrBgEFBQcDAQYI\n"
    "KwYBBQUHAwIwFAYDVR0RBA0wC4IJbG9jYWxob3N0MB0GA1UdDgQWBBTVi9ghasnS\n"
    "9xAoMgQD1xCpC+P4bjAfBgNVHSMEGDAWgBRYlzmzsiaT2WJ+pSkYicmZB5ac3DAK\n"
    "BggqhkjOPQQDAgNIADBFAiEAs+plZEfTCEeJ4jcmzjgS96yfdO8x3is670MZT/Sn\n"
    "RVYCIEliPkn74q7O2ctDiaJC1fvl0m5bBzFRyhA/R93j0gYN\n"
    "-----END CERTIFICATE-----\n";

/* The P-256 leaf key, SEC 1 with an embedded compressed public key. */
static const char fio___test_ecc_p256_key[] =
    "-----BEGIN EC PRIVATE KEY-----\n"
    "MFcCAQEEIMk3T/9EK4A1oMSIS3hjLGoYW+zR2EiiHjleffz1bOdJoAoGCCqGSM49\n"
    "AwEHoSQDIgADKgkG/hKWQD7RbHnKlhRc3/v090c4nDyPQAYphRAAmns=\n"
    "-----END EC PRIVATE KEY-----\n";

/* The same key, PKCS#8 (the inner SEC 1 key keeps the compressed point). */
static const char fio___test_ecc_p256_key_pkcs8[] =
    "-----BEGIN PRIVATE KEY-----\n"
    "MGcCAQAwEwYHKoZIzj0CAQYIKoZIzj0DAQcETTBLAgEBBCDJN0//RCuANaDEiEt4\n"
    "YyxqGFvs0dhIoh45Xn389WznSaEkAyIAAyoJBv4SlkA+0Wx5ypYUXN/79PdHOJw8\n"
    "j0AGKYUQAJp7\n"
    "-----END PRIVATE KEY-----\n";

/* The P-256 leaf scalar with the CA's public key embedded (mismatch). */
static const char fio___test_ecc_p256_key_badpub[] =
    "-----BEGIN EC PRIVATE KEY-----\n"
    "MFcCAQEEIMk3T/9EK4A1oMSIS3hjLGoYW+zR2EiiHjleffz1bOdJoAoGCCqGSM49\n"
    "AwEHoSQDIgACmgzwsIRa+4mDnvfXsQDLTqfX9K7g2u5W/knHFe5j1E4=\n"
    "-----END EC PRIVATE KEY-----\n";

/* P-384 CA, self-signed, compressed point (prefix 02). */
static const char fio___test_ecc_p384_ca[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIBhzCCAQ2gAwIBAgICA4QwCgYIKoZIzj0EAwMwIzEhMB8GA1UEAwwYQ29tcHJl\n"
    "c3NlZCBQLTM4NCBUZXN0IENBMCAXDTI1MDEwMTAwMDAwMFoYDzIxMjUxMjMxMjM1\n"
    "OTU5WjAjMSEwHwYDVQQDDBhDb21wcmVzc2VkIFAtMzg0IFRlc3QgQ0EwRjAQBgcq\n"
    "hkjOPQIBBgUrgQQAIgMyAAL251Bg+gFtYCNB+CZ1IpT5eoCJE5hoLgEEQsfrHTMy\n"
    "Mg4uuEphuk76vfH7K7d8MUmjQjBAMA8GA1UdEwEB/wQFMAMBAf8wDgYDVR0PAQH/\n"
    "BAQDAgEGMB0GA1UdDgQWBBTxMnCZ+/eeXMIuyuiAmEqhJAQShTAKBggqhkjOPQQD\n"
    "AwNoADBlAjEAyWf47CupGXu4wdJSiTUBiRF2jsOUUuxFq9QMKHV6K7nyFPyhuAlw\n"
    "oge6k04jddCdAjAXX8q1Fqyyf3IfXpFEkBvR4JIbGCyeR52KjdJm606RNTYoCfWS\n"
    "vpt9ps+3I74kPVk=\n"
    "-----END CERTIFICATE-----\n";

/* P-384 leaf (CN=localhost) issued by the P-384 CA, compressed (prefix 03). */
static const char fio___test_ecc_p384_leaf[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIBzTCCAVOgAwIBAgICE4QwCgYIKoZIzj0EAwMwIzEhMB8GA1UEAwwYQ29tcHJl\n"
    "c3NlZCBQLTM4NCBUZXN0IENBMCAXDTI1MDEwMTAwMDAwMFoYDzIxMjUxMjMxMjM1\n"
    "OTU5WjAUMRIwEAYDVQQDDAlsb2NhbGhvc3QwRjAQBgcqhkjOPQIBBgUrgQQAIgMy\n"
    "AAPofX0ZG0h95xmQTErQmhnnLx6Y/aSu06LKUi9kqNM7VtaNw+4bGIW/uRmb+FV2\n"
    "nEijgZYwgZMwDAYDVR0TAQH/BAIwADAOBgNVHQ8BAf8EBAMCB4AwHQYDVR0lBBYw\n"
    "FAYIKwYBBQUHAwEGCCsGAQUFBwMCMBQGA1UdEQQNMAuCCWxvY2FsaG9zdDAdBgNV\n"
    "HQ4EFgQUO0ZRxUkbrMTGzsqoXFH6c10dTZEwHwYDVR0jBBgwFoAU8TJwmfv3nlzC\n"
    "LsrogJhKoSQEEoUwCgYIKoZIzj0EAwMDaAAwZQIxAKUmJVvosUr44zvPwkzwbWA2\n"
    "PU6Vcq8L3gnrz+aTL/4deI29qeGRpxYBi/TljqCqVQIwIjlDqcdzInMyWk7Ntid3\n"
    "tOoB4axhbkSEpQCKBEywFcLgDvDhULlIwVa1b98fcoaf\n"
    "-----END CERTIFICATE-----\n";

/* The P-384 leaf key, SEC 1 with an embedded compressed public key. */
static const char fio___test_ecc_p384_key[] =
    "-----BEGIN EC PRIVATE KEY-----\n"
    "MHQCAQEEMBWXZ0R7HTWNE01x4uu2W2mt55VFOI9dhh1lGN2GhQIohrL2fiQImlHY\n"
    "ZSbX0eA7qKAHBgUrgQQAIqE0AzIAA+h9fRkbSH3nGZBMStCaGecvHpj9pK7TospS\n"
    "L2So0ztW1o3D7hsYhb+5GZv4VXacSA==\n"
    "-----END EC PRIVATE KEY-----\n";

/* The P-384 leaf key, PKCS#8 (the inner SEC 1 key keeps the compressed point,
 * drops the curve parameters). */
static const char fio___test_ecc_p384_key_pkcs8[] =
    "-----BEGIN PRIVATE KEY-----\n"
    "MIGEAgEAMBAGByqGSM49AgEGBSuBBAAiBG0wawIBAQQwFZdnRHsdNY0TTXHi67Zb\n"
    "aa3nlUU4j12GHWUY3YaFAiiGsvZ+JAiaUdhlJtfR4DuooTQDMgAD6H19GRtIfecZ\n"
    "kExK0JoZ5y8emP2krtOiylIvZKjTO1bWjcPuGxiFv7kZm/hVdpxI\n"
    "-----END PRIVATE KEY-----\n";

/* The P-384 leaf scalar with the CA's public key embedded (mismatch). */
static const char fio___test_ecc_p384_key_badpub[] =
    "-----BEGIN EC PRIVATE KEY-----\n"
    "MHQCAQEEMBWXZ0R7HTWNE01x4uu2W2mt55VFOI9dhh1lGN2GhQIohrL2fiQImlHY\n"
    "ZSbX0eA7qKAHBgUrgQQAIqE0AzIAAvbnUGD6AW1gI0H4JnUilPl6gIkTmGguAQRC\n"
    "x+sdMzIyDi64SmG6Tvq98fsrt3wxSQ==\n"
    "-----END EC PRIVATE KEY-----\n";

/* P-384 CA, self-signed, uncompressed point (04). */
static const char fio___test_ecc_p384u_ca[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIBuzCCAUGgAwIBAgICA4UwCgYIKoZIzj0EAwMwJTEjMCEGA1UEAwwaVW5jb21w\n"
    "cmVzc2VkIFAtMzg0IFRlc3QgQ0EwIBcNMjUwMTAxMDAwMDAwWhgPMjEyNTEyMzEy\n"
    "MzU5NTlaMCUxIzAhBgNVBAMMGlVuY29tcHJlc3NlZCBQLTM4NCBUZXN0IENBMHYw\n"
    "EAYHKoZIzj0CAQYFK4EEACIDYgAEXVlXuUKqm1W8wfXjv3kPI5hIIQJUSWDGHHly\n"
    "kw1PEwYN10qa/7j6kHi0AfDATMsyiQkZz+k7mSN13tBeStu5kocXLMXOtb0N4a56\n"
    "hGMfo+FDfMyiZBynZ+C1OFc7++Eto0IwQDAPBgNVHRMBAf8EBTADAQH/MA4GA1Ud\n"
    "DwEB/wQEAwIBBjAdBgNVHQ4EFgQUCYMS7XmOextDfE/3bcWSzu/xEoQwCgYIKoZI\n"
    "zj0EAwMDaAAwZQIwTQJ+9eHsVCPQS9MKuYcNgc/Q5gVG80u3nBqivgKuFVYma3u6\n"
    "dFL71sexzMmkAC+uAjEAvsWbkLuYBvaAx7Wd50NDSyKSFc1KebDqwe6Q5s43FKcX\n"
    "8EWbCrf47Aik2rncsR5O\n"
    "-----END CERTIFICATE-----\n";

/* P-384 leaf (CN=localhost) issued by the uncompressed CA, uncompressed. */
static const char fio___test_ecc_p384u_leaf[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIB/zCCAYWgAwIBAgICE4UwCgYIKoZIzj0EAwMwJTEjMCEGA1UEAwwaVW5jb21w\n"
    "cmVzc2VkIFAtMzg0IFRlc3QgQ0EwIBcNMjUwMTAxMDAwMDAwWhgPMjEyNTEyMzEy\n"
    "MzU5NTlaMBQxEjAQBgNVBAMMCWxvY2FsaG9zdDB2MBAGByqGSM49AgEGBSuBBAAi\n"
    "A2IABAi7qiJKR5zlBr2zeoSuz4EgM4d2uXZ5fV6YlEckp7IjvFxx8k7lJ9leNpgQ\n"
    "mQx8e66brkM7nsaNF+cXCJNCiLUXCD/D4ZcO70cDX5gArknnrWi8d8CbMFfYsfOO\n"
    "+IufL6OBljCBkzAMBgNVHRMBAf8EAjAAMA4GA1UdDwEB/wQEAwIHgDAdBgNVHSUE\n"
    "FjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwFAYDVR0RBA0wC4IJbG9jYWxob3N0MB0G\n"
    "A1UdDgQWBBRJAiDZsYIBm7IGDjc6zXo+yFtHLTAfBgNVHSMEGDAWgBQJgxLteY57\n"
    "G0N8T/dtxZLO7/EShDAKBggqhkjOPQQDAwNoADBlAjEAkreT4WC1Jb9+1N+IF2CQ\n"
    "djpCqDm5TGyRx/wY6wj3y7rMCTnvHZi+gquDw9Rmo1WXAjAVIOSPjKytYcBkfG/u\n"
    "o+EAsxzlJfwyDxq/VWLtYokAbMBAwr7C82qBHMkKVzsIfIo=\n"
    "-----END CERTIFICATE-----\n";

/* The uncompressed P-384 leaf key, SEC 1 with its uncompressed public key. */
static const char fio___test_ecc_p384u_key[] =
    "-----BEGIN EC PRIVATE KEY-----\n"
    "MIGkAgEBBDB+Kf75Au6ffwAHIUkGTM6iSh9EIDgNKLJmJjDSfFb+r9t+dzDnGMfe\n"
    "vRlZzpY9X4igBwYFK4EEACKhZANiAAQIu6oiSkec5Qa9s3qErs+BIDOHdrl2eX1e\n"
    "mJRHJKeyI7xccfJO5SfZXjaYEJkMfHuum65DO57GjRfnFwiTQoi1Fwg/w+GXDu9H\n"
    "A1+YAK5J561ovHfAmzBX2LHzjviLny8=\n"
    "-----END EC PRIVATE KEY-----\n";

/* The same key, PKCS#8. */
static const char fio___test_ecc_p384u_key_pkcs8[] =
    "-----BEGIN PRIVATE KEY-----\n"
    "MIG2AgEAMBAGByqGSM49AgEGBSuBBAAiBIGeMIGbAgEBBDB+Kf75Au6ffwAHIUkG\n"
    "TM6iSh9EIDgNKLJmJjDSfFb+r9t+dzDnGMfevRlZzpY9X4ihZANiAAQIu6oiSkec\n"
    "5Qa9s3qErs+BIDOHdrl2eX1emJRHJKeyI7xccfJO5SfZXjaYEJkMfHuum65DO57G\n"
    "jRfnFwiTQoi1Fwg/w+GXDu9HA1+YAK5J561ovHfAmzBX2LHzjviLny8=\n"
    "-----END PRIVATE KEY-----\n";

#endif /* H___FIO_TEST_EC_COMPRESSED_FIXTURES_H */
