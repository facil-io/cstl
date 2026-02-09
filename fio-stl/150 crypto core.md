## Cryptographic Core

```c
#define FIO_CRYPTO_CORE
#include FIO_INCLUDE_FILE
```

The Cryptographic Core module defines the standard function pointer types used as the AEAD (Authenticated Encryption with Associated Data) interface throughout the facil.io library. These types establish a uniform calling convention for all symmetric AEAD cipher implementations.

This module is automatically included as a dependency when defining any cryptographic module (`FIO_CHACHA`, `FIO_SHA1`, `FIO_SHA2`, `FIO_BLAKE2`, etc.). It rarely needs to be included directly.

**Note**: do NOT use these cryptographic primitives unless you have no other choice. Always prefer tested cryptographic libraries such as OpenSSL.

### AEAD Function Pointer Types

The two function pointer types below define the standard signature for all AEAD encryption and decryption functions in the library. Any function matching these signatures can be used interchangeably wherever the library expects an AEAD cipher — for example, when passed to `fio_x25519_encrypt` or `fio_x25519_decrypt`.

Concrete implementations that conform to these types include:

- `fio_chacha20_poly1305_enc` / `fio_chacha20_poly1305_dec` (ChaCha20-Poly1305)
- `fio_xchacha20_poly1305_enc` / `fio_xchacha20_poly1305_dec` (XChaCha20-Poly1305)
- `fio_aes128_gcm_enc` / `fio_aes128_gcm_dec` (AES-128-GCM)
- `fio_aes256_gcm_enc` / `fio_aes256_gcm_dec` (AES-256-GCM)

#### `fio_crypto_enc_fn`

```c
typedef void(fio_crypto_enc_fn)(void *restrict mac,
                                void *restrict data,
                                size_t len,
                                const void *ad,    /* additional data */
                                size_t adlen,
                                const void *key,
                                const void *nonce);
```

Function pointer type for AEAD encryption.

Performs in-place encryption of `data` and writes a message authentication code (MAC) to `mac`. The additional data (`ad`) is authenticated but **not** encrypted.

**Parameters:**
- `mac` - output buffer for the authentication tag (must have at least 16 bytes available)
- `data` - the plaintext to encrypt in-place (may be NULL if `len` is 0)
- `len` - length of the data in bytes
- `ad` - additional data to authenticate but not encrypt (may be NULL if `adlen` is 0)
- `adlen` - length of the additional data in bytes
- `key` - pointer to the encryption key (size depends on the concrete cipher, typically 32 bytes)
- `nonce` - pointer to the nonce/IV (size depends on the concrete cipher, typically 12 or 24 bytes)

**Note**: the exact sizes required for `key` and `nonce` depend on the concrete implementation. For example, ChaCha20-Poly1305 requires a 256-bit key (32 bytes) and a 96-bit nonce (12 bytes), while XChaCha20-Poly1305 requires a 192-bit nonce (24 bytes).

#### `fio_crypto_dec_fn`

```c
typedef int(fio_crypto_dec_fn)(void *restrict mac,
                               void *restrict data,
                               size_t len,
                               const void *ad,    /* additional data */
                               size_t adlen,
                               const void *key,
                               const void *nonce);
```

Function pointer type for AEAD decryption with authentication.

Authenticates the message using the MAC, then performs in-place decryption of `data`. The additional data (`ad`) must match the value used during encryption for authentication to succeed.

**Parameters:**
- `mac` - pointer to the authentication tag to verify (must point to at least 16 bytes)
- `data` - the ciphertext to decrypt in-place (may be NULL if `len` is 0)
- `len` - length of the data in bytes
- `ad` - additional data used during encryption (may be NULL only if originally omitted)
- `adlen` - length of the additional data in bytes
- `key` - pointer to the decryption key (same key used for encryption)
- `nonce` - pointer to the nonce/IV (same nonce used for encryption)

**Returns:** `0` on success (authentication passed, data decrypted), `-1` on error (authentication failed, data left unchanged or zeroed depending on implementation).

**Note**: if authentication fails, the data should be considered untrusted. Implementations typically leave the data in an undefined state or zero it on failure.

### Example

```c
#define FIO_CHACHA
#include FIO_INCLUDE_FILE

void example_aead_abstraction(void) {
  /* Use function pointers for cipher-agnostic code */
  fio_crypto_enc_fn *encrypt = fio_chacha20_poly1305_enc;
  fio_crypto_dec_fn *decrypt = fio_chacha20_poly1305_dec;

  uint8_t key[32] = {/* ... key material ... */};
  uint8_t nonce[12] = {/* ... nonce ... */};
  uint8_t mac[16];
  char message[] = "Hello, AEAD!";
  const char *ad = "additional data";

  /* Encrypt in-place */
  encrypt(mac, message, sizeof(message) - 1,
          ad, strlen(ad), key, nonce);

  /* Decrypt in-place and verify */
  int ok = decrypt(mac, message, sizeof(message) - 1,
                   ad, strlen(ad), key, nonce);
  if (ok == -1) {
    /* authentication failed */
  }
}
```

------------------------------------------------------------
