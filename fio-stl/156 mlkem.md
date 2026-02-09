## ML-KEM-768

```c
#define FIO_MLKEM
#include FIO_INCLUDE_FILE
```

By defining `FIO_MLKEM`, the ML-KEM-768 post-quantum key encapsulation mechanism and the X25519MLKEM768 hybrid key exchange are defined and made available.

ML-KEM (Module-Lattice-Based Key-Encapsulation Mechanism) is standardized in [FIPS 203](https://csrc.nist.gov/publications/detail/fips/203/final). ML-KEM-768 provides 192-bit security against both classical and quantum adversaries.

This module also provides X25519MLKEM768, the hybrid key exchange combining classical X25519 with post-quantum ML-KEM-768, as specified in [draft-ietf-tls-ecdhe-mlkem](https://datatracker.ietf.org/doc/draft-ietf-tls-ecdhe-mlkem/) for TLS 1.3 (NamedGroup 0x11ec).

**Warning**: this implementation has not been audited. Use at your own risk.

**Note**: X25519MLKEM768 requires `FIO_ED25519` for X25519 support.

### ML-KEM-768 Constants

#### `FIO_MLKEM768_PUBLICKEYBYTES`

```c
#define FIO_MLKEM768_PUBLICKEYBYTES  1184
```

Size of an ML-KEM-768 public (encapsulation) key in bytes.

#### `FIO_MLKEM768_SECRETKEYBYTES`

```c
#define FIO_MLKEM768_SECRETKEYBYTES  2400
```

Size of an ML-KEM-768 secret (decapsulation) key in bytes.

#### `FIO_MLKEM768_CIPHERTEXTBYTES`

```c
#define FIO_MLKEM768_CIPHERTEXTBYTES 1088
```

Size of an ML-KEM-768 ciphertext in bytes.

#### `FIO_MLKEM768_SSBYTES`

```c
#define FIO_MLKEM768_SSBYTES         32
```

Size of the ML-KEM-768 shared secret in bytes.

### ML-KEM-768 Functions

#### `fio_mlkem768_keypair`

```c
int fio_mlkem768_keypair(uint8_t pk[1184], uint8_t sk[2400]);
```

Generates an ML-KEM-768 keypair using the system CSPRNG.

**Parameters:**
- `pk` - output buffer for the public key (1184 bytes)
- `sk` - output buffer for the secret key (2400 bytes)

**Returns:** 0 on success, -1 on failure.

#### `fio_mlkem768_keypair_derand`

```c
int fio_mlkem768_keypair_derand(uint8_t pk[1184],
                                uint8_t sk[2400],
                                const uint8_t coins[64]);
```

Generates an ML-KEM-768 keypair from a deterministic 64-byte seed.

This function is primarily useful for testing with known test vectors (e.g., NIST ACVP vectors). For production use, prefer `fio_mlkem768_keypair`.

**Parameters:**
- `pk` - output buffer for the public key (1184 bytes)
- `sk` - output buffer for the secret key (2400 bytes)
- `coins` - deterministic seed: 32 bytes of `d` followed by 32 bytes of `z`

**Returns:** 0 on success, -1 on failure.

#### `fio_mlkem768_encaps`

```c
int fio_mlkem768_encaps(uint8_t ct[1088],
                        uint8_t ss[32],
                        const uint8_t pk[1184]);
```

Encapsulates a shared secret using the recipient's public key.

Generates a random shared secret and encrypts it into a ciphertext that only the holder of the corresponding secret key can decapsulate.

**Parameters:**
- `ct` - output buffer for the ciphertext (1088 bytes)
- `ss` - output buffer for the shared secret (32 bytes)
- `pk` - the recipient's public key (1184 bytes)

**Returns:** 0 on success, -1 on failure.

#### `fio_mlkem768_encaps_derand`

```c
int fio_mlkem768_encaps_derand(uint8_t ct[1088],
                               uint8_t ss[32],
                               const uint8_t pk[1184],
                               const uint8_t coins[32]);
```

Encapsulates with deterministic randomness. Primarily for testing with known test vectors.

**Parameters:**
- `ct` - output buffer for the ciphertext (1088 bytes)
- `ss` - output buffer for the shared secret (32 bytes)
- `pk` - the recipient's public key (1184 bytes)
- `coins` - deterministic 32-byte randomness

**Returns:** 0 on success, -1 on failure.

#### `fio_mlkem768_decaps`

```c
int fio_mlkem768_decaps(uint8_t ss[32],
                        const uint8_t ct[1088],
                        const uint8_t sk[2400]);
```

Decapsulates a ciphertext to recover the shared secret.

Uses **implicit rejection**: if the ciphertext is invalid, a pseudorandom shared secret is returned (derived from the secret key and ciphertext) rather than an error. This prevents chosen-ciphertext attacks — the caller cannot distinguish valid from invalid ciphertexts.

**Parameters:**
- `ss` - output buffer for the shared secret (32 bytes)
- `ct` - the ciphertext to decapsulate (1088 bytes)
- `sk` - the secret key (2400 bytes)

**Returns:** 0 (always succeeds for well-formed inputs).

### ML-KEM-768 Example

```c
#define FIO_MLKEM
#include FIO_INCLUDE_FILE

void example_mlkem768(void) {
  uint8_t pk[1184], sk[2400];
  uint8_t ct[1088];
  uint8_t ss_enc[32], ss_dec[32];

  /* Generate keypair */
  fio_mlkem768_keypair(pk, sk);

  /* Encapsulate (sender side) */
  fio_mlkem768_encaps(ct, ss_enc, pk);
  /* ss_enc is the shared secret; ct is sent to the recipient */

  /* Decapsulate (recipient side) */
  fio_mlkem768_decaps(ss_dec, ct, sk);
  /* ss_dec == ss_enc (both parties now share the same secret) */
}
```

### X25519MLKEM768 Hybrid Key Exchange

X25519MLKEM768 combines classical X25519 (Curve25519 Diffie-Hellman) with post-quantum ML-KEM-768 to provide security against both classical and quantum adversaries. If either component remains secure, the combined key exchange is secure.

This is the hybrid key exchange specified for TLS 1.3 in [draft-ietf-tls-ecdhe-mlkem](https://datatracker.ietf.org/doc/draft-ietf-tls-ecdhe-mlkem/) (NamedGroup 0x11ec).

**Note**: the group name "X25519MLKEM768" does NOT reflect the concatenation order. The ML-KEM component comes FIRST in all concatenations (per Section 4 of the draft).

### X25519MLKEM768 Constants

#### `FIO_X25519MLKEM768_PUBLICKEYBYTES`

```c
#define FIO_X25519MLKEM768_PUBLICKEYBYTES  1216 /* ML-KEM-768_ek (1184) + X25519_pk (32) */
```

Size of an X25519MLKEM768 hybrid public key in bytes.

#### `FIO_X25519MLKEM768_SECRETKEYBYTES`

```c
#define FIO_X25519MLKEM768_SECRETKEYBYTES  2432 /* ML-KEM-768_dk (2400) + X25519_sk (32) */
```

Size of an X25519MLKEM768 hybrid secret key in bytes.

#### `FIO_X25519MLKEM768_CIPHERTEXTBYTES`

```c
#define FIO_X25519MLKEM768_CIPHERTEXTBYTES 1120 /* ML-KEM-768_ct (1088) + X25519_ephemeral_pk (32) */
```

Size of an X25519MLKEM768 hybrid ciphertext in bytes.

#### `FIO_X25519MLKEM768_SSBYTES`

```c
#define FIO_X25519MLKEM768_SSBYTES         64   /* ML-KEM-768_ss (32) + X25519_ss (32) */
```

Size of the X25519MLKEM768 hybrid shared secret in bytes.

### X25519MLKEM768 Key Format

| Component | Public Key | Secret Key | Ciphertext | Shared Secret |
|-----------|-----------|-----------|-----------|--------------|
| ML-KEM-768 | 1184 bytes (first) | 2400 bytes (first) | 1088 bytes (first) | 32 bytes (first) |
| X25519 | 32 bytes (last) | 32 bytes (last) | 32 bytes (last) | 32 bytes (last) |
| **Total** | **1216 bytes** | **2432 bytes** | **1120 bytes** | **64 bytes** |

### X25519MLKEM768 Functions

#### `fio_x25519mlkem768_keypair`

```c
int fio_x25519mlkem768_keypair(uint8_t pk[1216], uint8_t sk[2432]);
```

Generates an X25519MLKEM768 hybrid keypair using the system CSPRNG.

Generates both an X25519 keypair and an ML-KEM-768 keypair. The public key is `ML-KEM-768_ek (1184) || X25519_pk (32)`. The secret key is `ML-KEM-768_dk (2400) || X25519_sk (32)`.

**Parameters:**
- `pk` - output buffer for the hybrid public key (1216 bytes)
- `sk` - output buffer for the hybrid secret key (2432 bytes)

**Returns:** 0 on success, -1 on failure.

#### `fio_x25519mlkem768_encaps`

```c
int fio_x25519mlkem768_encaps(uint8_t ct[1120],
                              uint8_t ss[64],
                              const uint8_t pk[1216]);
```

Performs X25519MLKEM768 hybrid encapsulation.

Performs both X25519 key exchange and ML-KEM-768 encapsulation against the recipient's hybrid public key. The ciphertext is `ML-KEM-768_ct (1088) || X25519_ephemeral_pk (32)`. The shared secret is `ML-KEM-768_ss (32) || X25519_ss (32)`.

**Parameters:**
- `ct` - output buffer for the hybrid ciphertext (1120 bytes)
- `ss` - output buffer for the hybrid shared secret (64 bytes)
- `pk` - the recipient's hybrid public key (1216 bytes)

**Returns:** 0 on success, -1 on failure.

#### `fio_x25519mlkem768_decaps`

```c
int fio_x25519mlkem768_decaps(uint8_t ss[64],
                              const uint8_t ct[1120],
                              const uint8_t sk[2432]);
```

Performs X25519MLKEM768 hybrid decapsulation.

Performs both X25519 shared secret derivation and ML-KEM-768 decapsulation. The shared secret is `ML-KEM-768_ss (32) || X25519_ss (32)`.

**Parameters:**
- `ss` - output buffer for the hybrid shared secret (64 bytes)
- `ct` - the hybrid ciphertext (1120 bytes)
- `sk` - the hybrid secret key (2432 bytes)

**Returns:** 0 on success, -1 if X25519 shared secret computation fails (low-order point). ML-KEM-768 uses implicit rejection for invalid ciphertexts.

### X25519MLKEM768 Example

```c
#define FIO_ED25519 /* Required for X25519 */
#define FIO_MLKEM
#include FIO_INCLUDE_FILE

void example_hybrid(void) {
  uint8_t pk[1216], sk[2432];
  uint8_t ct[1120];
  uint8_t ss_enc[64], ss_dec[64];

  /* Generate hybrid keypair (server side) */
  fio_x25519mlkem768_keypair(pk, sk);

  /* Encapsulate (client side) */
  fio_x25519mlkem768_encaps(ct, ss_enc, pk);
  /* ss_enc[0..63] is the 64-byte hybrid shared secret */
  /* ct[0..1119] is sent to the server */

  /* Decapsulate (server side) */
  fio_x25519mlkem768_decaps(ss_dec, ct, sk);
  /* ss_dec == ss_enc (both parties share the same 64-byte secret) */
  /* Typically fed into a KDF (e.g., HKDF) for key derivation */
}
```

### ML-KEM-768 Parameters

| Parameter | Value | Description |
|-----------|-------|-------------|
| n | 256 | Polynomial degree |
| k | 3 | Module rank (number of polynomials) |
| q | 3329 | Modulus |
| eta1 | 2 | Noise parameter for key generation |
| eta2 | 2 | Noise parameter for encryption |
| d_u | 10 | Compression bits for ciphertext vector |
| d_v | 4 | Compression bits for ciphertext scalar |

### Browser and Server Support

X25519MLKEM768 (TLS NamedGroup 0x11ec) is supported by:
- Chrome 131+
- Firefox 132+
- Safari / iOS 26+
- ~8.6% of top 1M websites (as of early 2026)

-------------------------------------------------------------------------------
