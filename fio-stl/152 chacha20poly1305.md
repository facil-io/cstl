# ChaCha20 & Poly1305

```c
#define FIO_CHACHA
#include "fio-stl.h"
```

ChaCha20 stream cipher, Poly1305 authenticator, and the ChaCha20-Poly1305 AEAD combination. Also includes XChaCha20-Poly1305 with a 192-bit nonce, which is safer for random nonces than the 96-bit variant.

**Security note:** this implementation has not been independently audited. Use at your own risk, and prefer a tested cryptographic library when one is available.

**Invalid arguments never abort or crash.** A `NULL` `key`, `nonce` or `mac`, or a `NULL` `data` / `ad` with a non-zero length, fails like over-limit input: functions return `-1` (encryption zeroes `data` and `mac`, decryption leaves `data` untouched, MAC-only functions zero a writable `mac`). `NULL` buffers with zero length are valid.

### ChaCha20-Poly1305 API

#### `fio_chacha20_poly1305_enc`

```c
SFUNC int fio_chacha20_poly1305_enc(void *restrict mac,
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
- `nonce` — 12-byte nonce; a key/nonce pair must never be reused.

**Returns:** `0` on success, `-1` if `len` exceeds `(2^32 - 1) * 64` bytes (the block counter would wrap and reuse keystream). On error no keystream is used: `data` and `mac` are zeroed, so no plaintext and no valid tag are produced. The process is never aborted.

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

**Returns:** `0` on success, `-1` if authentication fails (`data` is zeroed) or if `len` exceeds the AEAD limit of `(2^32 - 1) * 64` bytes (`data` is left untouched).

#### `fio_chacha20_poly1305_auth`

```c
SFUNC int fio_chacha20_poly1305_auth(void *restrict mac,
                                     void *restrict data,
                                     size_t len,
                                     const void *ad,
                                     size_t adlen,
                                     const void *key,
                                     const void *nonce);
```

Computes the Poly1305 tag for already-encrypted data without decrypting it.

**Returns:** `0` on success, `-1` on invalid arguments or if `len` exceeds the AEAD limit of `(2^32 - 1) * 64` bytes. On error a writable `mac` is zeroed; reject the message on `-1` and never compare the cleared tag.

### Standalone ChaCha20 / Poly1305

#### `fio_chacha20`

```c
SFUNC int fio_chacha20(void *restrict data,
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

**Returns:** `0` on success, `-1` if the block counter would wrap (`counter + ceil(len / 64)` exceeds `2^32`). On error no keystream is reused and `data` is zeroed (it may hold plaintext).

#### `fio_poly1305_auth`

```c
SFUNC int fio_poly1305_auth(void *restrict mac_dest,
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
- `ad` — additional data; may be `NULL` if `ad_len` is 0.
- `ad_len` — additional data length.
- `key256bits` — 32-byte one-time Poly1305 key.

**Returns:** `0` on success, `-1` on invalid arguments (`NULL` `mac_dest` or key, or `NULL` `message` / `ad` with a non-zero length). On error nothing is read and a writable `mac_dest` is zeroed (never a MAC from a fallback key); reject the message on `-1` and never compare the cleared tag.

### XChaCha20-Poly1305 API

XChaCha20 uses a 24-byte nonce, large enough for nonces generated by a cryptographically secure RNG (collision odds are negligible, roughly `2^-96` after `2^48` messages under one key). A key/nonce pair must never be reused.

#### `fio_xchacha20_poly1305_enc`

```c
SFUNC int fio_xchacha20_poly1305_enc(void *restrict mac,
                                     void *restrict data,
                                     size_t len,
                                     const void *ad,
                                     size_t adlen,
                                     const void *key,
                                     const void *nonce);
```

Same shape and return value as `fio_chacha20_poly1305_enc`, but `nonce` must be 24 bytes.

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
SFUNC int fio_xchacha20(void *restrict data,
                        size_t len,
                        const void *key,
                        const void *nonce,
                        uint32_t counter);
```

In-place XChaCha20 encryption or decryption. Returns `0` on success, `-1` (with `data` zeroed) if the block counter would wrap, same as `fio_chacha20`.

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

  if (fio_chacha20_poly1305_enc(tag, msg, sizeof(msg), NULL, 0, key, nonce))
    return 1; /* input too long - never happens for small messages */

  if (fio_chacha20_poly1305_dec(tag, msg, sizeof(msg), NULL, 0, key, nonce)) {
    printf("authentication failed\n");
    return 1;
  }
  return 0;
}
```

------------------------------------------------------------
