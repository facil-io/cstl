# HKDF

```c
#define FIO_HKDF
#include "fio-stl.h"
```

HMAC-based Key Derivation Function per [RFC 5869](https://www.rfc-editor.org/rfc/rfc5869). HKDF extracts entropy from input keying material into a pseudorandom key, then expands that key into one or more output keys.

Supports SHA-256 and SHA-384 as the underlying HMAC.

**Note:** HKDF needs `fio_sha256_hmac` and `fio_sha512_hmac`. Define `FIO_SHA2` before `FIO_HKDF`, or use `FIO_CRYPTO` to pull in everything.

### Constants

#### `FIO_HKDF_SHA256_HASH_LEN`

```c
#define FIO_HKDF_SHA256_HASH_LEN 32
```

SHA-256 hash length in bytes.

#### `FIO_HKDF_SHA384_HASH_LEN`

```c
#define FIO_HKDF_SHA384_HASH_LEN 48
```

SHA-384 hash length in bytes.

### Functions

#### `fio_hkdf_extract`

```c
SFUNC void fio_hkdf_extract(void *restrict prk,
                            const void *restrict salt,
                            size_t salt_len,
                            const void *restrict ikm,
                            size_t ikm_len,
                            int use_sha384);
```

HKDF-Extract: `PRK = HMAC-Hash(salt, IKM)`.

**Parameters:**
- `prk` — output buffer (32 bytes for SHA-256, 48 for SHA-384).
- `salt` — optional salt; if `NULL`, a zero string of hash length is used.
- `salt_len` — salt length.
- `ikm` — input keying material.
- `ikm_len` — IKM length.
- `use_sha384` — non-zero for SHA-384, zero for SHA-256.

#### `fio_hkdf_expand`

```c
SFUNC void fio_hkdf_expand(void *restrict okm,
                           size_t okm_len,
                           const void *restrict prk,
                           size_t prk_len,
                           const void *restrict info,
                           size_t info_len,
                           int use_sha384);
```

HKDF-Expand: `OKM = HKDF-Expand(PRK, info, L)`.

**Parameters:**
- `okm` — output buffer.
- `okm_len` — desired output length (max `255 * hash_len`).
- `prk` — pseudorandom key from Extract.
- `prk_len` — 32 for SHA-256, 48 for SHA-384.
- `info` — optional context/info string; may be `NULL`.
- `info_len` — info length.
- `use_sha384` — non-zero for SHA-384, zero for SHA-256.

#### `fio_hkdf`

```c
SFUNC void fio_hkdf(void *restrict okm,
                    size_t okm_len,
                    const void *restrict salt,
                    size_t salt_len,
                    const void *restrict ikm,
                    size_t ikm_len,
                    const void *restrict info,
                    size_t info_len,
                    int use_sha384);
```

Combined HKDF: Extract followed by Expand in one call.

### Example

```c
#define FIO_SHA2
#define FIO_HKDF
#include "fio-stl.h"

int main(void) {
  const char *ikm = "input keying material";
  const char *salt = "salt";
  const char *info = "app context";
  uint8_t key[32];

  fio_hkdf(key, sizeof(key), salt, strlen(salt),
           ikm, strlen(ikm), info, strlen(info), 0);
  return 0;
}
```

------------------------------------------------------------
