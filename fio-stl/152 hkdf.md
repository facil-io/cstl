## HKDF

```c
#define FIO_HKDF
#include FIO_INCLUDE_FILE
```

By defining `FIO_HKDF`, the HMAC-based Key Derivation Function (HKDF) is defined and made available.

HKDF is standardized in [RFC 5869](https://www.rfc-editor.org/rfc/rfc5869) and provides a two-stage key derivation process:

1. **Extract** - concentrates entropy from input keying material into a fixed-length pseudorandom key (PRK)
2. **Expand** - derives one or more output keys from the PRK

Supports both SHA-256 and SHA-384 as the underlying HMAC hash function.

**Note**: HKDF requires SHA-2 HMAC functions (`fio_sha256_hmac`, `fio_sha512_hmac`). Either define `FIO_SHA2` before `FIO_HKDF`, or use `FIO_CRYPTO` to include all crypto modules.

### HKDF Constants

#### `FIO_HKDF_SHA256_HASH_LEN`

```c
#define FIO_HKDF_SHA256_HASH_LEN 32
```

SHA-256 hash output length in bytes.

#### `FIO_HKDF_SHA384_HASH_LEN`

```c
#define FIO_HKDF_SHA384_HASH_LEN 48
```

SHA-384 hash output length in bytes.

### HKDF Functions

#### `fio_hkdf_extract`

```c
void fio_hkdf_extract(void *restrict prk,
                      const void *restrict salt,
                      size_t salt_len,
                      const void *restrict ikm,
                      size_t ikm_len,
                      int use_sha384);
```

HKDF-Extract: computes `PRK = HMAC-Hash(salt, IKM)`.

Extracts a pseudorandom key (PRK) from input keying material (IKM). The PRK concentrates the entropy from the IKM into a fixed-length output suitable for use with HKDF-Expand.

**Parameters:**
- `prk` - output buffer for the pseudorandom key (32 bytes for SHA-256, 48 bytes for SHA-384)
- `salt` - optional salt value (if NULL, uses a string of zeros of hash length)
- `salt_len` - length of the salt in bytes
- `ikm` - input keying material
- `ikm_len` - length of the IKM in bytes
- `use_sha384` - if non-zero, use SHA-384; otherwise use SHA-256

#### `fio_hkdf_expand`

```c
void fio_hkdf_expand(void *restrict okm,
                     size_t okm_len,
                     const void *restrict prk,
                     size_t prk_len,
                     const void *restrict info,
                     size_t info_len,
                     int use_sha384);
```

HKDF-Expand: computes `OKM = HKDF-Expand(PRK, info, L)`.

Expands a pseudorandom key (PRK) into output keying material (OKM) of the desired length. The `info` parameter provides application-specific context to derive distinct keys from the same PRK.

**Parameters:**
- `okm` - output buffer for the derived keying material
- `okm_len` - desired output length in bytes (maximum 255 * hash_len)
- `prk` - pseudorandom key from HKDF-Extract (32 or 48 bytes)
- `prk_len` - length of the PRK (32 for SHA-256, 48 for SHA-384)
- `info` - optional context and application-specific information
- `info_len` - length of the info in bytes
- `use_sha384` - if non-zero, use SHA-384; otherwise use SHA-256

#### `fio_hkdf`

```c
void fio_hkdf(void *restrict okm,
              size_t okm_len,
              const void *restrict salt,
              size_t salt_len,
              const void *restrict ikm,
              size_t ikm_len,
              const void *restrict info,
              size_t info_len,
              int use_sha384);
```

Combined HKDF (Extract + Expand) in a single call.

Derives keying material from input keying material using the full HKDF process (RFC 5869 Section 2). Equivalent to calling `fio_hkdf_extract` followed by `fio_hkdf_expand`.

**Parameters:**
- `okm` - output buffer for the derived keying material
- `okm_len` - desired output length in bytes (maximum 255 * hash_len)
- `salt` - optional salt value (if NULL, uses a string of zeros of hash length)
- `salt_len` - length of the salt in bytes
- `ikm` - input keying material
- `ikm_len` - length of the IKM in bytes
- `info` - optional context and application-specific information
- `info_len` - length of the info in bytes
- `use_sha384` - if non-zero, use SHA-384; otherwise use SHA-256

### HKDF Examples

#### Basic Key Derivation

```c
#define FIO_SHA2
#define FIO_HKDF
#include FIO_INCLUDE_FILE

void example_hkdf(void) {
  const char *ikm = "input keying material";
  const char *salt = "optional salt";
  const char *info = "application context";
  uint8_t derived_key[32];

  /* Derive a 32-byte key using SHA-256 */
  fio_hkdf(derived_key, 32,
           salt, strlen(salt),
           ikm, strlen(ikm),
           info, strlen(info),
           0 /* use SHA-256 */);
}
```

#### Two-Stage Key Derivation

```c
void example_two_stage(void) {
  const char *ikm = "shared secret from key exchange";
  uint8_t prk[48]; /* SHA-384 PRK */

  /* Extract: concentrate entropy */
  fio_hkdf_extract(prk, NULL, 0,
                   ikm, strlen(ikm),
                   1 /* use SHA-384 */);

  /* Expand: derive multiple keys from same PRK */
  uint8_t client_key[32], server_key[32];
  fio_hkdf_expand(client_key, 32, prk, 48,
                  "client key", 10, 1);
  fio_hkdf_expand(server_key, 32, prk, 48,
                  "server key", 10, 1);
}
```

-------------------------------------------------------------------------------
