# ML-KEM-768

```c
#define FIO_MLKEM
#include "fio-stl.h"
```

ML-KEM-768 is the FIPS 203 module-lattice key encapsulation mechanism: a post-quantum way to agree on a shared secret. This header also exposes the TLS hybrid `X25519MLKEM768` shape, which combines ML-KEM-768 with X25519.

**Security note:** this implementation has not been independently audited. Use it carefully, and prefer audited crypto stacks for high-value deployments.

## ML-KEM-768 Sizes

```c
#define FIO_MLKEM768_PUBLICKEYBYTES  1184
#define FIO_MLKEM768_SECRETKEYBYTES  2400
#define FIO_MLKEM768_CIPHERTEXTBYTES 1088
#define FIO_MLKEM768_SSBYTES         32
#define FIO_MLKEM768_SYMBYTES        32
```

Parameters: `n = 256`, `k = 3`, `q = 3329`, `eta1 = 2`, `eta2 = 2`, `d_u = 10`, `d_v = 4`.

## ML-KEM-768 API

### `fio_mlkem768_keypair`

```c
SFUNC int fio_mlkem768_keypair(uint8_t pk[1184], uint8_t sk[2400]);
```

Generates a random ML-KEM-768 key pair using the system CSPRNG. Returns `0` on success, `-1` on failure.

### `fio_mlkem768_keypair_derand`

```c
SFUNC int fio_mlkem768_keypair_derand(uint8_t pk[1184],
                                      uint8_t sk[2400],
                                      const uint8_t coins[64]);
```

Generates a deterministic key pair. `coins` is exactly 64 bytes: `d || z`. This is mostly for test vectors and reproducible checks; production code should normally use `fio_mlkem768_keypair`.

### `fio_mlkem768_encaps`

```c
SFUNC int fio_mlkem768_encaps(uint8_t ct[1088],
                              uint8_t ss[32],
                              const uint8_t pk[1184]);
```

Creates a ciphertext and shared secret for the holder of `pk`. Randomness comes from the system CSPRNG. Send `ct`; keep `ss` as the shared secret input to your KDF or protocol.

### `fio_mlkem768_encaps_derand`

```c
SFUNC int fio_mlkem768_encaps_derand(uint8_t ct[1088],
                                     uint8_t ss[32],
                                     const uint8_t pk[1184],
                                     const uint8_t coins[32]);
```

Deterministic encapsulation using 32 bytes of caller-provided randomness. Useful for known-answer tests. Boring in production, which is exactly the point.

### `fio_mlkem768_decaps`

```c
SFUNC int fio_mlkem768_decaps(uint8_t ss[32],
                              const uint8_t ct[1088],
                              const uint8_t sk[2400]);
```

Recovers the shared secret from a ciphertext and secret key.

ML-KEM uses implicit rejection: invalid ciphertexts produce a pseudorandom shared secret derived from the secret key and ciphertext instead of a loud failure. That prevents chosen-ciphertext games from turning your error path into an oracle.

Returns `0` for well-formed inputs.

## X25519MLKEM768 Hybrid

`X25519MLKEM768` is the TLS 1.3 hybrid key exchange shape from the ECDHE/ML-KEM draft. It needs `FIO_ED25519` too, because this STL implements X25519 in the Ed25519/Curve25519 header.

The name starts with X25519, but the bytes start with ML-KEM. Yes, naming is hard.

```c
#define FIO_X25519MLKEM768_PUBLICKEYBYTES  (32 + 1184) /* 1216 */
#define FIO_X25519MLKEM768_SECRETKEYBYTES  (32 + 2400) /* 2432 */
#define FIO_X25519MLKEM768_CIPHERTEXTBYTES (32 + 1088) /* 1120 */
#define FIO_X25519MLKEM768_SSBYTES         64
```

| Value | Layout |
| --- | --- |
| Public key | `ML-KEM-768_ek` (1184) `||` `X25519_pk` (32) |
| Secret key | `ML-KEM-768_dk` (2400) `||` `X25519_sk` (32) |
| Ciphertext | `ML-KEM-768_ct` (1088) `||` `X25519_ephemeral_pk` (32) |
| Shared secret | `ML-KEM-768_ss` (32) `||` `X25519_ss` (32) |

### `fio_x25519mlkem768_keypair`

```c
SFUNC int fio_x25519mlkem768_keypair(uint8_t pk[1216], uint8_t sk[2432]);
```

Generates both the ML-KEM-768 and X25519 key pairs with system randomness. Returns `0` on success.

### `fio_x25519mlkem768_encaps`

```c
SFUNC int fio_x25519mlkem768_encaps(uint8_t ct[1120],
                                    uint8_t ss[64],
                                    const uint8_t pk[1216]);
```

Performs ML-KEM encapsulation and X25519 ephemeral key exchange. The returned shared secret is the concatenation `ML-KEM-768_ss || X25519_ss`.

### `fio_x25519mlkem768_decaps`

```c
SFUNC int fio_x25519mlkem768_decaps(uint8_t ss[64],
                                    const uint8_t ct[1120],
                                    const uint8_t sk[2432]);
```

Decapsulates the ML-KEM part and computes the X25519 shared secret. Returns `0` on success, or `-1` if X25519 rejects the peer point. ML-KEM invalid ciphertexts still use implicit rejection.

## Example

```c
#define FIO_MLKEM
#include "fio-stl.h"

int roundtrip(void) {
  uint8_t pk[FIO_MLKEM768_PUBLICKEYBYTES];
  uint8_t sk[FIO_MLKEM768_SECRETKEYBYTES];
  uint8_t ct[FIO_MLKEM768_CIPHERTEXTBYTES];
  uint8_t sender_ss[FIO_MLKEM768_SSBYTES];
  uint8_t receiver_ss[FIO_MLKEM768_SSBYTES];

  if (fio_mlkem768_keypair(pk, sk))
    return -1;
  if (fio_mlkem768_encaps(ct, sender_ss, pk))
    return -1;
  if (fio_mlkem768_decaps(receiver_ss, ct, sk))
    return -1;

  return fio_memcmp(sender_ss, receiver_ss, sizeof(sender_ss));
}
```

## Implementation Notes

The implementation uses ML-KEM-768 parameters, NTT arithmetic, Montgomery and Barrett reduction, and optional NEON / AVX2 paths for vectorized polynomial work. Those SIMD paths are speed knobs, not different APIs.

------------------------------------------------------------
