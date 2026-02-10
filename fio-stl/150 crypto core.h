/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_CRYPTO_CORE        /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                          Cryptographic Core Module




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_CRYPTO_CORE) && !defined(H___FIO_CRYPTO_CORE___H)
/* *****************************************************************************
**Note**: do NOT use these cryptographic unless you have no other choice. Always
*prefer tested cryptographic libraries such as OpenSSL.
***************************************************************************** */
#define H___FIO_CRYPTO_CORE___H

/* *****************************************************************************
AEAD Function Pointer Types

These types define the interface for authenticated encryption with associated
data (AEAD) ciphers. Used by TLS and other protocols to abstract cipher choice.

Implementations:
- fio_chacha20_poly1305_enc/dec (152 chacha20poly1305.h)
- fio_xchacha20_poly1305_enc/dec (152 chacha20poly1305.h)
- fio_aes128_gcm_enc/dec (153 aes.h)
- fio_aes256_gcm_enc/dec (153 aes.h)
***************************************************************************** */

typedef void(fio_crypto_enc_fn)(void *restrict mac,
                                void *restrict data,
                                size_t len,
                                const void *ad, /* additional data */
                                size_t adlen,
                                const void *key,
                                const void *nonce);
typedef int(fio_crypto_dec_fn)(void *restrict mac,
                               void *restrict data,
                               size_t len,
                               const void *ad, /* additional data */
                               size_t adlen,
                               const void *key,
                               const void *nonce);

/* *****************************************************************************
SIMD Optimization Notes for Crypto Modules

This section documents the SIMD patterns used across crypto modules. The
primitives are kept module-specific rather than shared because:

1. Each algorithm has unique constants (moduli, reduction parameters)
2. Parameterization would add overhead in hot paths
3. The patterns are simple enough that duplication is acceptable

SIMD Detection Macros (defined in 000 core.h):
- FIO___HAS_ARM_INTRIN: ARM NEON available (includes arm_neon.h)
- FIO___HAS_X86_INTRIN: x86-64 intrinsics available (includes immintrin.h)
- __AVX2__: AVX2 256-bit SIMD available (check with FIO___HAS_X86_INTRIN)

Module-Specific SIMD Implementations:

ChaCha20Poly1305 (152 chacha20poly1305.h):
- NEON: 4-block parallel (256 bytes/call), vertical SIMD layout
- AVX2: 8-block parallel (512 bytes/call), vertical SIMD layout
- Uses byte shuffle for 8/16-bit rotations, shift+or for 7/12-bit

Ed25519/X25519 (154 ed25519.h):
- NEON: Vectorized 5-limb field add/sub/cswap for GF(2^255-19)
- AVX2: Vectorized 5-limb field add/sub/cswap for GF(2^255-19)
- Multiplication stays scalar (128-bit multiply is already fast)

ML-KEM-768 (156 mlkem.h):
- NEON: Vectorized NTT/InvNTT (8-wide int16x8_t), Montgomery/Barrett reduction
- AVX2: Vectorized NTT/InvNTT (16-wide __m256i), Montgomery/Barrett reduction
- Uses q=3329, QINV=-3327, Barrett constant=20159

Common SIMD Patterns:

1. Constant-Time Conditional Swap (cswap):
   mask = 0 - (uint64_t)condition;  // All 1s if true, all 0s if false
   t = mask & (a ^ b);
   a ^= t; b ^= t;

2. Montgomery Reduction (for small prime q):
   t = (int16_t)(a * QINV);         // Low 16 bits of a * q^-1
   result = (a - t * q) >> 16;      // Exact division by 2^16

3. Barrett Reduction (for small prime q):
   v = ceil(2^k / q);               // Precomputed constant
   t = (v * a + 2^(k-1)) >> k;      // Approximate quotient
   result = a - t * q;              // Remainder

4. Vertical SIMD Layout (for block ciphers):
   Each SIMD register holds the same word position from multiple blocks.
   v[w] = {block0[w], block1[w], ..., blockN[w]}
   Enables parallel processing of independent blocks.
***************************************************************************** */

/* *****************************************************************************
Module Implementation - possibly externed functions.
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/* *****************************************************************************
Module Cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_CRYPTO_CORE
#endif /* FIO_CRYPTO_CORE */
