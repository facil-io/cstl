## ChaCha20 & Poly1305

```c
#define FIO_CHACHA
#include "fio-stl.h"
```

Non-streaming ChaCha20 and Poly1305 implementations are provided for cases when a cryptography library isn't available (or too heavy) but a good enough symmetric cryptographic solution is required. Please note that this implementation was not tested from a cryptographic viewpoint and although constant time was desired it might not have been achieved on all systems / CPUs.

**Note:** some CPUs do not offer constant time MUL and might leak information through side-chain attacks.

#### `fio_chacha20_poly1305_enc`

```c
void fio_chacha20_poly1305_enc(void *mac,
                               void *data,
                               size_t len,
                               void *ad, /* additional data */
                               size_t adlen,
                               void *key,
                               void *nounce);
```

Performs an in-place encryption of `data` using ChaCha20 with additional data, producing a 16 byte message authentication code (MAC) using Poly1305.

* `key`    MUST point to a 256 bit long memory address (32 Bytes).
* `nounce` MUST point to a  96 bit long memory address (12 Bytes).
* `ad`     MAY be omitted, will NOT be encrypted.
* `data`   MAY be omitted, WILL be encrypted.
* `mac`    MUST point to a buffer with (at least) 16 available bytes.

#### `fio_chacha20_poly1305_dec`

```c
int fio_chacha20_poly1305_dec(void *mac,
                              void *data,
                              size_t len,
                              void *ad, /* additional data */
                              size_t adlen,
                              void *key,
                              void *nounce);
```

Performs an in-place decryption of `data` using ChaCha20 after authenticating the message authentication code (MAC) using Poly1305.

* `key`    MUST point to a 256 bit long memory address (32 Bytes).
* `nounce` MUST point to a  96 bit long memory address (12 Bytes).
* `ad`     MAY be omitted ONLY IF originally omitted.
* `data`   MAY be omitted, WILL be decrypted.
* `mac`    MUST point to a buffer where the 16 byte MAC is placed.

Returns `-1` on error (authentication failed).

#### `fio_chacha20`

```c
void fio_chacha20(void *data,
                  size_t len,
                  void *key,
                  void *nounce,
                  uint32_t counter);
```

Performs an in-place encryption/decryption of `data` using ChaCha20.

* `key`    MUST point to a 256 bit long memory address (32 Bytes).
* `nounce` MUST point to a  96 bit long memory address (12 Bytes).
* `counter` is the block counter, usually 1 unless `data` is mid-cyphertext.


#### `fio_poly1305_auth`

```c
void fio_poly1305_auth(void *mac_dest,
                       void *key256bits,
                       void *message,
                       size_t len,
                       void *additional_data,
                       size_t additional_data_len);
```

Given a Poly1305 256bit (16 byte) key, writes the Poly1305 authentication code for the message and additional data into `mac_dest`.

* `key`    MUST point to a 256 bit long memory address (32 Bytes).

-------------------------------------------------------------------------------
