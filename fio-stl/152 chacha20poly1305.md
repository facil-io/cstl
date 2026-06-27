# ChaCha20 & Poly1305

```c
#define FIO_CHACHA
#include "fio-stl.h"
```

ChaCha20 stream cipher, Poly1305 authenticator, and the ChaCha20-Poly1305 AEAD combination. Also includes XChaCha20-Poly1305 with a 192-bit nonce, which is safer for random nonces than the 96-bit variant.

**Security note:** this implementation has not been independently audited. Use at your own risk, and prefer a tested cryptographic library when one is available.

### ChaCha20-Poly1305 API

#### `fio_chacha20_poly1305_enc`

```c
SFUNC void fio_chacha20_poly1305_enc(void *restrict mac,
                                     void *restrict data,
                                     size_t len,
                                     const void *ad,
                                     size_t adlen,
                                     const void *key,
                                     const void *nonce);
```

In-place encryption with a 16-byte Poly1305 tag.

**Parameters:**
- `mac` — output buffer; must have at least 16 writable bytes.
- `data` — plaintext buffer; encrypted in place.
- `len` — data length.
- `ad` — additional authenticated data (not encrypted); may be `NULL`.
- `adlen` — length of `ad`.
- `key` — 32-byte key.
- `nonce` — 12-byte nonce.

#### `fio_chacha20_poly1305_dec`

```c
SFUNC int fio_chacha20_poly1305_dec(void *restrict mac,
                                    void *restrict data,
                                    size_t len,
                                    const void *ad,
                                    size_t adlen,
                                    const void *key,
                                    const void *nonce);
```

In-place decryption with tag verification.

**Returns:** `0` on success, `-1` if authentication fails. On failure `data` is zeroed.

#### `fio_chacha20_poly1305_auth`

```c
SFUNC void fio_chacha20_poly1305_auth(void *restrict mac,
                                      void *restrict data,
                                      size_t len,
                                      const void *ad,
                                      size_t adlen,
                                      const void *key,
                                      const void *nonce);
```

Computes the Poly1305 tag for already-encrypted data without decrypting it.

### Standalone ChaCha20 / Poly1305

#### `fio_chacha20`

```c
SFUNC void fio_chacha20(void *restrict data,
                        size_t len,
                        const void *key,
                        const void *nonce,
                        uint32_t counter);
```

In-place ChaCha20 encryption or decryption.

**Parameters:**
- `data` — buffer to transform in place.
- `len` — buffer length.
- `key` — 32-byte key.
- `nonce` — 12-byte nonce.
- `counter` — block counter; usually `1` unless resuming mid-ciphertext.

#### `fio_poly1305_auth`

```c
SFUNC void fio_poly1305_auth(void *restrict mac_dest,
                             void *restrict message,
                             size_t len,
                             const void *ad,
                             size_t ad_len,
                             const void *key256bits);
```

Computes a Poly1305 MAC for `message` and `ad` using a 32-byte key.

**Parameters:**
- `mac_dest` — output buffer; must have at least 16 writable bytes.
- `message` — message to authenticate; may be `NULL` if `len` is 0.
- `len` — message length.
- `ad` — additional data; may be `NULL`.
- `ad_len` — additional data length.
- `key256bits` — 32-byte Poly1305 key.

### XChaCha20-Poly1305 API

XChaCha20 uses a 24-byte nonce, making random nonces safe from birthday-bound collisions.

#### `fio_xchacha20_poly1305_enc`

```c
SFUNC void fio_xchacha20_poly1305_enc(void *restrict mac,
                                      void *restrict data,
                                      size_t len,
                                      const void *ad,
                                      size_t adlen,
                                      const void *key,
                                      const void *nonce);
```

Same shape as `fio_chacha20_poly1305_enc`, but `nonce` must be 24 bytes.

#### `fio_xchacha20_poly1305_dec`

```c
SFUNC int fio_xchacha20_poly1305_dec(void *restrict mac,
                                     void *restrict data,
                                     size_t len,
                                     const void *ad,
                                     size_t adlen,
                                     const void *key,
                                     const void *nonce);
```

Same shape as `fio_chacha20_poly1305_dec`, but `nonce` must be 24 bytes.

#### `fio_xchacha20`

```c
SFUNC void fio_xchacha20(void *restrict data,
                         size_t len,
                         const void *key,
                         const void *nonce,
                         uint32_t counter);
```

In-place XChaCha20 encryption or decryption.

### Example

```c
#define FIO_CHACHA
#include "fio-stl.h"

int main(void) {
  uint8_t key[32] = {0};
  uint8_t nonce[12] = {0};
  uint8_t msg[32] = "hello, chacha world!";
  uint8_t tag[16];

  fio_rand_bytes_secure(key, sizeof(key));
  fio_rand_bytes_secure(nonce, sizeof(nonce));

  fio_chacha20_poly1305_enc(tag, msg, sizeof(msg), NULL, 0, key, nonce);

  if (fio_chacha20_poly1305_dec(tag, msg, sizeof(msg), NULL, 0, key, nonce)) {
    printf("authentication failed\n");
    return 1;
  }
  return 0;
}
```

------------------------------------------------------------
