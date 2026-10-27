# HKDF

```c
#define FIO_HKDF
#include "fio-stl.h"
```

HMAC-based Key Derivation Function per [RFC 5869](https://www.rfc-editor.org/rfc/rfc5869). HKDF extracts entropy from input keying material into a pseudorandom key, then expands that key into one or more output keys.

Supports SHA-256 and SHA-384 as the underlying HMAC.

**Note:** HKDF needs the `FIO_SHA2` module (SHA-256 / SHA-384 and their HMAC). Define `FIO_SHA2` before `FIO_HKDF`, or use `FIO_CRYPTO` to pull in everything.

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
SFUNC int fio_hkdf_extract(void *restrict prk,
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
- `ikm` — input keying material; may be `NULL` only when `ikm_len` is 0.
- `ikm_len` — IKM length.
- `use_sha384` — non-zero for SHA-384, zero for SHA-256.

Returns `0` on success, or `-1` (PRK not written) if `prk` is `NULL` or `ikm` is `NULL` with a non-zero length.

#### `fio_hkdf_expand`

```c
SFUNC int fio_hkdf_expand(void *restrict okm,
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
- `prk_len` — PRK length; at least 32 for SHA-256, 48 for SHA-384 (RFC 5869 §2.3).
- `info` — optional context/info string; `NULL` is empty info (`info_len` ignored).
- `info_len` — info length.
- `use_sha384` — non-zero for SHA-384, zero for SHA-256.

Returns `0` on success. Returns `-1` without writing `okm` if `okm_len > 255 * hash_len`, `prk_len < hash_len`, `prk` is `NULL`, or `okm` is `NULL` with a non-zero length. Expand does not allocate memory and wipes its intermediate secrets.

#### `fio_hkdf`

```c
SFUNC int fio_hkdf(void *restrict okm,
                   size_t okm_len,
                   const void *restrict salt,
                   size_t salt_len,
                   const void *restrict ikm,
                   size_t ikm_len,
                   const void *restrict info,
                   size_t info_len,
                   int use_sha384);
```

Combined HKDF: Extract followed by Expand in one call. Returns `0` on success, or `-1` without writing `okm` on any argument error listed above. The intermediate PRK is wiped.

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

  if (fio_hkdf(key, sizeof(key), salt, strlen(salt),
               ikm, strlen(ikm), info, strlen(info), 0))
    return 1;
  return 0;
}
```

------------------------------------------------------------
