## ChaCha20 & Poly1305

```c
#define FIO_CHACHA
#include "fio-stl.h"
```

Non-streaming ChaCha20 and Poly1305 implementations are provided for cases when a cryptography library isn't available (or too heavy) but a good enough symmetric cryptographic solution is required. Please note that this implementation was not tested from a cryptographic viewpoint and although constant time was desired it might not have been achieved on all systems / CPUs.

**Note:** some CPUs do not offer constant time MUL and might leak information through side-chain attacks.

**Note:** this module depends on the `FIO_MATH` module which will be automatically included.

### ChaCha20Poly1305 API

#### `fio_chacha20_poly1305_enc`

```c
void fio_chacha20_poly1305_enc(void *restrict mac,
                               void *restrict data,
                               size_t len,
                               const void *ad, /* additional data */
                               size_t adlen,
                               const void *key,
                               const void *nonce);
```

Performs an in-place encryption of `data` using ChaCha20 with additional data, producing a 16 byte message authentication code (MAC) using Poly1305.

* `mac`    MUST point to a buffer with (at least) 16 available bytes.
* `data`   MAY be omitted, WILL be encrypted.
* `len`    length of `data` in bytes.
* `ad`     MAY be omitted, will NOT be encrypted.
* `adlen`  length of `ad` in bytes.
* `key`    MUST point to a 256 bit long memory address (32 Bytes).
* `nonce`  MUST point to a  96 bit long memory address (12 Bytes).

#### `fio_chacha20_poly1305_dec`

```c
int fio_chacha20_poly1305_dec(void *restrict mac,
                              void *restrict data,
                              size_t len,
                              const void *ad, /* additional data */
                              size_t adlen,
                              const void *key,
                              const void *nonce);
```

Performs an in-place decryption of `data` using ChaCha20 after authenticating the message authentication code (MAC) using Poly1305.

* `mac`    MUST point to a buffer where the 16 byte MAC is placed.
* `data`   MAY be omitted, WILL be decrypted.
* `len`    length of `data` in bytes.
* `ad`     MAY be omitted ONLY IF originally omitted.
* `adlen`  length of `ad` in bytes.
* `key`    MUST point to a 256 bit long memory address (32 Bytes).
* `nonce`  MUST point to a  96 bit long memory address (12 Bytes).

**Returns:** `0` on success, `-1` on error (authentication failed).

#### `fio_chacha20_poly1305_auth`

```c
void fio_chacha20_poly1305_auth(void *restrict mac,
                                void *restrict data,
                                size_t len,
                                const void *ad, /* additional data */
                                size_t adlen,
                                const void *key,
                                const void *nonce);
```

Computes the Poly1305 authentication tag for already-encrypted data without performing decryption.

This function is useful when you need to verify or compute the MAC for ciphertext that was encrypted using ChaCha20Poly1305, without decrypting the data.

* `mac`    MUST point to a buffer with (at least) 16 available bytes for the computed MAC.
* `data`   the encrypted data (ciphertext).
* `len`    length of `data` in bytes.
* `ad`     additional authenticated data (MAY be omitted).
* `adlen`  length of `ad` in bytes.
* `key`    MUST point to a 256 bit long memory address (32 Bytes).
* `nonce`  MUST point to a  96 bit long memory address (12 Bytes).

### Using ChaCha20 and Poly1305 Separately

#### `fio_chacha20`

```c
void fio_chacha20(void *restrict data,
                  size_t len,
                  const void *key,
                  const void *nonce,
                  uint32_t counter);
```

Performs an in-place encryption/decryption of `data` using ChaCha20.

* `data`    the data to encrypt/decrypt in-place.
* `len`     length of `data` in bytes.
* `key`     MUST point to a 256 bit long memory address (32 Bytes).
* `nonce`   MUST point to a  96 bit long memory address (12 Bytes).
* `counter` is the block counter, usually 1 unless `data` is mid-cyphertext.

#### `fio_poly1305_auth`

```c
void fio_poly1305_auth(void *restrict mac,
                       const void *key,
                       void *restrict msg,
                       size_t len,
                       const void *ad,
                       size_t ad_len);
```

Given a Poly1305 256bit (32 byte) key, writes the Poly1305 authentication code for the message and additional data into `mac`.

* `mac`    MUST point to a 128 bit long memory address (16 Bytes).
* `key`    MUST point to a 256 bit long memory address (32 Bytes).
* `msg`    the message to authenticate.
* `len`    length of `msg` in bytes.
* `ad`     additional data to authenticate (MAY be omitted).
* `ad_len` length of `ad` in bytes.

### XChaCha20-Poly1305 API (Extended Nonce)

XChaCha20-Poly1305 is the extended-nonce variant that uses a 192-bit (24-byte) nonce instead of the standard 96-bit (12-byte) nonce. This makes it safe to use randomly-generated nonces without collision risk due to the birthday paradox.

**How it works:**
1. HChaCha20 derives a 256-bit subkey from the original key and first 16 bytes of the nonce
2. Standard ChaCha20-Poly1305 is applied using the subkey and remaining 8 bytes of the nonce

#### `fio_xchacha20_poly1305_enc`

```c
void fio_xchacha20_poly1305_enc(void *restrict mac,
                                void *restrict data,
                                size_t len,
                                const void *ad,
                                size_t adlen,
                                const void *key,
                                const void *nonce);
```

Performs an in-place encryption of `data` using XChaCha20 with additional data, producing a 16 byte message authentication code (MAC) using Poly1305.

* `mac`    MUST point to a buffer with (at least) 16 available bytes.
* `data`   MAY be omitted, WILL be encrypted.
* `len`    length of `data` in bytes.
* `ad`     MAY be omitted, will NOT be encrypted.
* `adlen`  length of `ad` in bytes.
* `key`    MUST point to a 256 bit long memory address (32 Bytes).
* `nonce`  MUST point to a 192 bit long memory address (24 Bytes).

#### `fio_xchacha20_poly1305_dec`

```c
int fio_xchacha20_poly1305_dec(void *restrict mac,
                               void *restrict data,
                               size_t len,
                               const void *ad,
                               size_t adlen,
                               const void *key,
                               const void *nonce);
```

Performs an in-place decryption of `data` using XChaCha20 after authenticating the message authentication code (MAC) using Poly1305.

* `mac`    MUST point to a buffer where the 16 byte MAC is placed.
* `data`   MAY be omitted, WILL be decrypted.
* `len`    length of `data` in bytes.
* `ad`     MAY be omitted ONLY IF originally omitted.
* `adlen`  length of `ad` in bytes.
* `key`    MUST point to a 256 bit long memory address (32 Bytes).
* `nonce`  MUST point to a 192 bit long memory address (24 Bytes).

**Returns:** `0` on success, `-1` on error (authentication failed).

#### `fio_xchacha20`

```c
void fio_xchacha20(void *restrict data,
                   size_t len,
                   const void *key,
                   const void *nonce,
                   uint32_t counter);
```

Performs an in-place encryption/decryption of `data` using XChaCha20.

* `data`    the data to encrypt/decrypt in-place.
* `len`     length of `data` in bytes.
* `key`     MUST point to a 256 bit long memory address (32 Bytes).
* `nonce`   MUST point to a 192 bit long memory address (24 Bytes).
* `counter` is the block counter, usually 0 unless `data` is mid-cyphertext.

-------------------------------------------------------------------------------
