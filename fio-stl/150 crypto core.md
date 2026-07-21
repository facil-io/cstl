# Crypto Core

```c
#define FIO_CRYPTO_CORE
#include "fio-stl.h"
```

The crypto slice is a small, zero-dependency toolbox for code that needs to hash, sign, encrypt, parse certificates, or derive keys without dragging in a larger library. Handy? yes. Magic security dust? no.

`FIO_CRYPTO_CORE` itself is the shared glue. It defines the AEAD function pointer types used by the symmetric ciphers and by higher-level helpers that accept “any AEAD with this call shape”. Most users include a specific module, or define `FIO_CRYPTO` to pull in the crypto set.

## Map of the Crypto Modules

| Class | Docs | What lives there |
| --- | --- | --- |
| Hash | [SHA-1](./152%20sha1.md), [SHA-2](./152%20sha2.md), [SHA-3 / SHAKE](./152%20sha3.md), [BLAKE2](./152%20blake2.md) | Digest functions, streaming hash contexts, SHA/BLAKE HMAC helpers where provided. SHA-1 is legacy-only glue. |
| Symmetric | [ChaCha20-Poly1305](./152%20chacha20poly1305.md), [AES-GCM](./153%20aes.md) | AEAD encryption, stream cipher helpers, and the shared in-place authenticated encryption shape. |
| Asymmetric | [Ed25519 & X25519](./154%20ed25519.md), [P-256](./154%20p256.md), [P-384](./154%20p384.md), [RSA](./155%20rsa.md) | Signatures, key exchange, ECIES-style X25519 encryption, and RSA signatures for TLS-style use cases. |
| PKI | [ASN.1 DER](./155%20der.md), [X.509](./155%20x509.md), [PEM](./156%20pem.md) | DER/PEM parsing, certificate fields, hostname checks, signature checks, and certificate chain validation helpers. |
| KDF | [HKDF](./152%20hkdf.md), [Argon2](./159%20argon2.md), [Lyra2](./159%20lyra2.md), [OTP](./159%20otp.md), [Secrets](./159%20secret.md) | Key derivation, password hashing, TOTP codes, and hashing a process secret into a stable internal value. |
| Post-Quantum | [ML-KEM-768](./156%20mlkem.md) | ML-KEM-768 key encapsulation and the X25519MLKEM768 hybrid key exchange shape used by TLS drafts. |

## The Shared AEAD Shape

The core header defines two function pointer types:

```c
typedef void(fio_crypto_enc_fn)(void *restrict mac,
                                void *restrict data,
                                size_t len,
                                const void *ad,
                                size_t adlen,
                                const void *key,
                                const void *nonce);

typedef int(fio_crypto_dec_fn)(void *restrict mac,
                               void *restrict data,
                               size_t len,
                               const void *ad,
                               size_t adlen,
                               const void *key,
                               const void *nonce);
```

Current AEAD implementations that match this shape include:

- `fio_chacha20_poly1305_enc` / `fio_chacha20_poly1305_dec`
- `fio_xchacha20_poly1305_enc` / `fio_xchacha20_poly1305_dec`
- `fio_aes128_gcm_enc` / `fio_aes128_gcm_dec`
- `fio_aes256_gcm_enc` / `fio_aes256_gcm_dec`

These functions encrypt or decrypt `data` in place. `ad` is authenticated but not encrypted. `mac` is the authentication tag buffer; the current AEAD modules use 16-byte tags. Key and nonce sizes belong to the selected cipher, not to the function pointer type.

Decryption returns `0` when authentication succeeds and `-1` when it fails. Treat any failure as “message not trusted”; do not parse, log in detail, or partially use the plaintext.

## Safe Use, Plainly

This module helps when you need portable crypto building blocks inside the STL: hashing buffers, deriving session keys, signing messages, checking certificates, or doing AEAD encryption with a known protocol design.

It does **not** promise that your protocol is safe. It does not manage long-term keys for you, pick nonces, rotate secrets, maintain a root trust store, protect keys in hardware, provide FIPS validation, or substitute for an external security review. Several public-key and PKI headers explicitly note that they have not been independently audited.

Use this toolbox when:

- you need the zero-dependency facil.io STL path;
- the protocol is already designed and reviewed;
- inputs, key sizes, nonce rules, and failure paths are controlled;
- a compact embedded copy is more important than delegating to a platform stack.

Prefer audited platform/security libraries such as OpenSSL, BoringSSL, libsodium, CommonCrypto, platform TLS, or OS key stores when:

- you are building TLS, PKI, payment, identity, or compliance-sensitive systems;
- private keys live for a long time or leave the process boundary;
- certificate trust decisions must track OS/browser policy;
- side-channel hardening, hardware acceleration policy, or formal validation matters.

In short: this is a sharp little knife. Useful. Keep fingers clear.

## Implementation Notes

The crypto headers use optional CPU-specific paths where available, including SHA intrinsics, AES-NI/PCLMULQDQ, ARM crypto extensions, NEON, and AVX2. The public APIs stay the same when the implementation falls back to portable C.

The SIMD notes in `150 crypto core.h` are documentation for maintainers: ChaCha20-Poly1305 can process blocks in parallel, Curve25519 uses vectorized field add/sub/cswap where useful, and ML-KEM vectorizes NTT/reduction paths. These are implementation details, not extra guarantees.
