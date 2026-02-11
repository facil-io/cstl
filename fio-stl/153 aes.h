/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_AES                /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                              AES & AES-GCM




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_AES) && !defined(H___FIO_AES___H)
#define H___FIO_AES___H 1

/* *****************************************************************************
AES-GCM API

AES-GCM provides authenticated encryption with associated data (AEAD).
This is the most common cipher suite for TLS 1.3.

Supported key sizes:
- AES-128-GCM: 16-byte key (128 bits)
- AES-256-GCM: 32-byte key (256 bits)

The nonce (IV) MUST be 12 bytes (96 bits) for GCM mode.
The authentication tag is 16 bytes (128 bits).

API matches fio_chacha20_poly1305 for easy function pointer substitution.
***************************************************************************** */

/**
 * Performs in-place AES-128-GCM encryption with authentication.
 *
 * * `mac`    MUST point to a buffer with (at least) 16 available bytes.
 * * `key`    MUST point to a 128 bit key (16 Bytes).
 * * `nonce`  MUST point to a 96 bit nonce (12 Bytes).
 * * `ad`     MAY be omitted, will NOT be encrypted.
 * * `data`   MAY be omitted, WILL be encrypted.
 */
SFUNC void fio_aes128_gcm_enc(void *restrict mac,
                              void *restrict data,
                              size_t len,
                              const void *ad,
                              size_t adlen,
                              const void *key,
                              const void *nonce);

/**
 * Performs in-place AES-128-GCM decryption with authentication verification.
 *
 * * `mac`    MUST point to a buffer with the 16 byte MAC to verify.
 * * `key`    MUST point to a 128 bit key (16 Bytes).
 * * `nonce`  MUST point to a 96 bit nonce (12 Bytes).
 * * `ad`     MAY be omitted ONLY IF originally omitted.
 * * `data`   MAY be omitted, WILL be decrypted.
 *
 * Returns `-1` on error (authentication failed).
 */
SFUNC int fio_aes128_gcm_dec(void *restrict mac,
                             void *restrict data,
                             size_t len,
                             const void *ad,
                             size_t adlen,
                             const void *key,
                             const void *nonce);

/**
 * Performs in-place AES-256-GCM encryption with authentication.
 *
 * * `mac`    MUST point to a buffer with (at least) 16 available bytes.
 * * `key`    MUST point to a 256 bit key (32 Bytes).
 * * `nonce`  MUST point to a 96 bit nonce (12 Bytes).
 * * `ad`     MAY be omitted, will NOT be encrypted.
 * * `data`   MAY be omitted, WILL be encrypted.
 */
SFUNC void fio_aes256_gcm_enc(void *restrict mac,
                              void *restrict data,
                              size_t len,
                              const void *ad,
                              size_t adlen,
                              const void *key,
                              const void *nonce);

/**
 * Performs in-place AES-256-GCM decryption with authentication verification.
 *
 * * `mac`    MUST point to a buffer with the 16 byte MAC to verify.
 * * `key`    MUST point to a 256 bit key (32 Bytes).
 * * `nonce`  MUST point to a 96 bit nonce (12 Bytes).
 * * `ad`     MAY be omitted ONLY IF originally omitted.
 * * `data`   MAY be omitted, WILL be decrypted.
 *
 * Returns `-1` on error (authentication failed).
 */
SFUNC int fio_aes256_gcm_dec(void *restrict mac,
                             void *restrict data,
                             size_t len,
                             const void *ad,
                             size_t adlen,
                             const void *key,
                             const void *nonce);

/* *****************************************************************************
AES-GCM Implementation
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/* *****************************************************************************
Hardware Intrinsics Detection
***************************************************************************** */

/* x86/x64 AES-NI + PCLMULQDQ detection */
#if defined(FIO___HAS_X86_INTRIN) && defined(__AES__) && defined(__PCLMUL__)
#define FIO___HAS_X86_AES_INTRIN 1
#endif

/* ARM Crypto Extensions detection */
#if defined(FIO___HAS_ARM_INTRIN)
#define FIO___HAS_ARM_AES_INTRIN 1
#endif

/* *****************************************************************************
x86 AES-NI Implementation
***************************************************************************** */
#if defined(FIO___HAS_X86_AES_INTRIN) && FIO___HAS_X86_AES_INTRIN

/* AES-128 key expansion using AES-NI */
FIO_IFUNC __m128i fio___aesni_key_expand_128(__m128i key, __m128i keygen) {
  keygen = _mm_shuffle_epi32(keygen, 0xFF);
  key = _mm_xor_si128(key, _mm_slli_si128(key, 4));
  key = _mm_xor_si128(key, _mm_slli_si128(key, 4));
  key = _mm_xor_si128(key, _mm_slli_si128(key, 4));
  return _mm_xor_si128(key, keygen);
}

FIO_IFUNC void fio___aesni_gcm128_init(__m128i *rk,
                                       __m128i *h,
                                       const uint8_t key[16]) {
  __m128i k = _mm_loadu_si128((const __m128i *)key);
  rk[0] = k;
  rk[1] =
      fio___aesni_key_expand_128(rk[0], _mm_aeskeygenassist_si128(rk[0], 0x01));
  rk[2] =
      fio___aesni_key_expand_128(rk[1], _mm_aeskeygenassist_si128(rk[1], 0x02));
  rk[3] =
      fio___aesni_key_expand_128(rk[2], _mm_aeskeygenassist_si128(rk[2], 0x04));
  rk[4] =
      fio___aesni_key_expand_128(rk[3], _mm_aeskeygenassist_si128(rk[3], 0x08));
  rk[5] =
      fio___aesni_key_expand_128(rk[4], _mm_aeskeygenassist_si128(rk[4], 0x10));
  rk[6] =
      fio___aesni_key_expand_128(rk[5], _mm_aeskeygenassist_si128(rk[5], 0x20));
  rk[7] =
      fio___aesni_key_expand_128(rk[6], _mm_aeskeygenassist_si128(rk[6], 0x40));
  rk[8] =
      fio___aesni_key_expand_128(rk[7], _mm_aeskeygenassist_si128(rk[7], 0x80));
  rk[9] =
      fio___aesni_key_expand_128(rk[8], _mm_aeskeygenassist_si128(rk[8], 0x1B));
  rk[10] =
      fio___aesni_key_expand_128(rk[9], _mm_aeskeygenassist_si128(rk[9], 0x36));

  /* Compute H = AES(K, 0^128) */
  __m128i zero = _mm_setzero_si128();
  __m128i tmp = _mm_xor_si128(zero, rk[0]);
  tmp = _mm_aesenc_si128(tmp, rk[1]);
  tmp = _mm_aesenc_si128(tmp, rk[2]);
  tmp = _mm_aesenc_si128(tmp, rk[3]);
  tmp = _mm_aesenc_si128(tmp, rk[4]);
  tmp = _mm_aesenc_si128(tmp, rk[5]);
  tmp = _mm_aesenc_si128(tmp, rk[6]);
  tmp = _mm_aesenc_si128(tmp, rk[7]);
  tmp = _mm_aesenc_si128(tmp, rk[8]);
  tmp = _mm_aesenc_si128(tmp, rk[9]);
  *h = _mm_aesenclast_si128(tmp, rk[10]);
}

/* AES-256 key expansion helpers */
FIO_IFUNC __m128i fio___aesni_key_expand_256_1(__m128i key, __m128i keygen) {
  keygen = _mm_shuffle_epi32(keygen, 0xFF);
  key = _mm_xor_si128(key, _mm_slli_si128(key, 4));
  key = _mm_xor_si128(key, _mm_slli_si128(key, 4));
  key = _mm_xor_si128(key, _mm_slli_si128(key, 4));
  return _mm_xor_si128(key, keygen);
}

FIO_IFUNC __m128i fio___aesni_key_expand_256_2(__m128i key, __m128i keygen) {
  keygen = _mm_shuffle_epi32(keygen, 0xAA);
  key = _mm_xor_si128(key, _mm_slli_si128(key, 4));
  key = _mm_xor_si128(key, _mm_slli_si128(key, 4));
  key = _mm_xor_si128(key, _mm_slli_si128(key, 4));
  return _mm_xor_si128(key, keygen);
}

FIO_IFUNC void fio___aesni_gcm256_init(__m128i *rk,
                                       __m128i *h,
                                       const uint8_t key[32]) {
  __m128i k1 = _mm_loadu_si128((const __m128i *)key);
  __m128i k2 = _mm_loadu_si128((const __m128i *)(key + 16));
  rk[0] = k1;
  rk[1] = k2;
  rk[2] = fio___aesni_key_expand_256_1(rk[0],
                                       _mm_aeskeygenassist_si128(rk[1], 0x01));
  rk[3] = fio___aesni_key_expand_256_2(rk[1],
                                       _mm_aeskeygenassist_si128(rk[2], 0x00));
  rk[4] = fio___aesni_key_expand_256_1(rk[2],
                                       _mm_aeskeygenassist_si128(rk[3], 0x02));
  rk[5] = fio___aesni_key_expand_256_2(rk[3],
                                       _mm_aeskeygenassist_si128(rk[4], 0x00));
  rk[6] = fio___aesni_key_expand_256_1(rk[4],
                                       _mm_aeskeygenassist_si128(rk[5], 0x04));
  rk[7] = fio___aesni_key_expand_256_2(rk[5],
                                       _mm_aeskeygenassist_si128(rk[6], 0x00));
  rk[8] = fio___aesni_key_expand_256_1(rk[6],
                                       _mm_aeskeygenassist_si128(rk[7], 0x08));
  rk[9] = fio___aesni_key_expand_256_2(rk[7],
                                       _mm_aeskeygenassist_si128(rk[8], 0x00));
  rk[10] = fio___aesni_key_expand_256_1(rk[8],
                                        _mm_aeskeygenassist_si128(rk[9], 0x10));
  rk[11] =
      fio___aesni_key_expand_256_2(rk[9],
                                   _mm_aeskeygenassist_si128(rk[10], 0x00));
  rk[12] =
      fio___aesni_key_expand_256_1(rk[10],
                                   _mm_aeskeygenassist_si128(rk[11], 0x20));
  rk[13] =
      fio___aesni_key_expand_256_2(rk[11],
                                   _mm_aeskeygenassist_si128(rk[12], 0x00));
  rk[14] =
      fio___aesni_key_expand_256_1(rk[12],
                                   _mm_aeskeygenassist_si128(rk[13], 0x40));

  /* Compute H = AES(K, 0^128) */
  __m128i zero = _mm_setzero_si128();
  __m128i tmp = _mm_xor_si128(zero, rk[0]);
  for (int i = 1; i < 14; ++i)
    tmp = _mm_aesenc_si128(tmp, rk[i]);
  *h = _mm_aesenclast_si128(tmp, rk[14]);
}

/* AES-NI block encryption */
FIO_IFUNC __m128i fio___aesni_encrypt128(__m128i block, const __m128i *rk) {
  block = _mm_xor_si128(block, rk[0]);
  block = _mm_aesenc_si128(block, rk[1]);
  block = _mm_aesenc_si128(block, rk[2]);
  block = _mm_aesenc_si128(block, rk[3]);
  block = _mm_aesenc_si128(block, rk[4]);
  block = _mm_aesenc_si128(block, rk[5]);
  block = _mm_aesenc_si128(block, rk[6]);
  block = _mm_aesenc_si128(block, rk[7]);
  block = _mm_aesenc_si128(block, rk[8]);
  block = _mm_aesenc_si128(block, rk[9]);
  return _mm_aesenclast_si128(block, rk[10]);
}

FIO_IFUNC __m128i fio___aesni_encrypt256(__m128i block, const __m128i *rk) {
  block = _mm_xor_si128(block, rk[0]);
  for (int i = 1; i < 14; ++i)
    block = _mm_aesenc_si128(block, rk[i]);
  return _mm_aesenclast_si128(block, rk[14]);
}

/* GHASH multiplication using PCLMULQDQ (carryless multiply) */
FIO_IFUNC __m128i fio___ghash_mult_pclmul(__m128i a, __m128i b) {
  /* Perform carryless multiplication */
  __m128i tmp2 = _mm_clmulepi64_si128(a, b, 0x00);
  __m128i tmp3 = _mm_clmulepi64_si128(a, b, 0x01);
  __m128i tmp4 = _mm_clmulepi64_si128(a, b, 0x10);
  __m128i tmp5 = _mm_clmulepi64_si128(a, b, 0x11);

  tmp3 = _mm_xor_si128(tmp3, tmp4);
  tmp4 = _mm_slli_si128(tmp3, 8);
  tmp3 = _mm_srli_si128(tmp3, 8);
  tmp2 = _mm_xor_si128(tmp2, tmp4);
  tmp5 = _mm_xor_si128(tmp5, tmp3);

  /* Reduction modulo x^128 + x^7 + x^2 + x + 1 */
  __m128i tmp6 = _mm_srli_epi32(tmp2, 31);
  __m128i tmp7 = _mm_srli_epi32(tmp2, 30);
  __m128i tmp8 = _mm_srli_epi32(tmp2, 25);
  tmp6 = _mm_xor_si128(tmp6, tmp7);
  tmp6 = _mm_xor_si128(tmp6, tmp8);
  tmp7 = _mm_shuffle_epi32(tmp6, 0x93);
  tmp6 = _mm_and_si128(tmp7, _mm_set_epi32(0, ~0, ~0, ~0));
  tmp7 = _mm_and_si128(tmp7, _mm_set_epi32(~0, 0, 0, 0));
  tmp2 = _mm_xor_si128(tmp2, tmp6);
  tmp5 = _mm_xor_si128(tmp5, tmp7);

  __m128i tmp9 = _mm_slli_epi32(tmp2, 1);
  tmp2 = _mm_xor_si128(tmp2, tmp9);
  tmp9 = _mm_slli_epi32(tmp2, 2);
  tmp2 = _mm_xor_si128(tmp2, tmp9);
  tmp9 = _mm_slli_epi32(tmp2, 7);
  tmp2 = _mm_xor_si128(tmp2, tmp9);
  tmp7 = _mm_srli_si128(tmp2, 12);
  tmp2 = _mm_slli_si128(tmp2, 4);
  tmp5 = _mm_xor_si128(tmp5, tmp2);
  tmp5 = _mm_xor_si128(tmp5, tmp7);

  return tmp5;
}

/* Byte-swap for GCM (big-endian) */
FIO_IFUNC __m128i fio___bswap128(__m128i x) {
  return _mm_shuffle_epi8(
      x,
      _mm_set_epi8(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15));
}

/**
 * Precompute H powers for parallel GHASH.
 * htbl[0] = H¹, htbl[1] = H², ..., htbl[n-1] = Hⁿ.
 * All stored in byte-swapped (GCM) form.
 *
 * When compute8 is true, computes H¹ through H⁸ (for 8-block GHASH).
 * When false, computes H¹ through H⁴ (for 4-block GHASH).
 */
FIO_IFUNC void fio___x86_ghash_precompute(__m128i h,
                                          __m128i *htbl,
                                          int compute8) {
  htbl[0] = h;                                         /* H¹ */
  htbl[1] = fio___ghash_mult_pclmul(h, h);             /* H² */
  htbl[2] = fio___ghash_mult_pclmul(htbl[1], h);       /* H³ */
  htbl[3] = fio___ghash_mult_pclmul(htbl[1], htbl[1]); /* H⁴ */
  if (compute8) {
    htbl[4] = fio___ghash_mult_pclmul(htbl[3], h);       /* H⁵ */
    htbl[5] = fio___ghash_mult_pclmul(htbl[3], htbl[1]); /* H⁶ */
    htbl[6] = fio___ghash_mult_pclmul(htbl[3], htbl[2]); /* H⁷ */
    htbl[7] = fio___ghash_mult_pclmul(htbl[3], htbl[3]); /* H⁸ */
  }
}

/* 4-way parallel GHASH with deferred reduction.
 * Accumulates Karatsuba partial products across all 4 multiplies,
 * then performs a single reduction. Saves 3 reductions per 4-block batch. */
FIO_IFUNC __m128i fio___x86_ghash_mult4(__m128i x0,
                                        __m128i x1,
                                        __m128i x2,
                                        __m128i x3,
                                        const __m128i *htbl) {
  /* Block 0: x0 * H^4 */
  __m128i lo = _mm_clmulepi64_si128(x0, htbl[3], 0x00);
  __m128i hi = _mm_clmulepi64_si128(x0, htbl[3], 0x11);
  __m128i m1 = _mm_clmulepi64_si128(x0, htbl[3], 0x01);
  __m128i m2 = _mm_clmulepi64_si128(x0, htbl[3], 0x10);

  /* Block 1: x1 * H^3 — accumulate via XOR */
  lo = _mm_xor_si128(lo, _mm_clmulepi64_si128(x1, htbl[2], 0x00));
  hi = _mm_xor_si128(hi, _mm_clmulepi64_si128(x1, htbl[2], 0x11));
  m1 = _mm_xor_si128(m1, _mm_clmulepi64_si128(x1, htbl[2], 0x01));
  m2 = _mm_xor_si128(m2, _mm_clmulepi64_si128(x1, htbl[2], 0x10));

  /* Block 2: x2 * H^2 */
  lo = _mm_xor_si128(lo, _mm_clmulepi64_si128(x2, htbl[1], 0x00));
  hi = _mm_xor_si128(hi, _mm_clmulepi64_si128(x2, htbl[1], 0x11));
  m1 = _mm_xor_si128(m1, _mm_clmulepi64_si128(x2, htbl[1], 0x01));
  m2 = _mm_xor_si128(m2, _mm_clmulepi64_si128(x2, htbl[1], 0x10));

  /* Block 3: x3 * H^1 */
  lo = _mm_xor_si128(lo, _mm_clmulepi64_si128(x3, htbl[0], 0x00));
  hi = _mm_xor_si128(hi, _mm_clmulepi64_si128(x3, htbl[0], 0x11));
  m1 = _mm_xor_si128(m1, _mm_clmulepi64_si128(x3, htbl[0], 0x01));
  m2 = _mm_xor_si128(m2, _mm_clmulepi64_si128(x3, htbl[0], 0x10));

  /* Single Karatsuba combination on accumulated sums */
  __m128i mid = _mm_xor_si128(m1, m2);
  lo = _mm_xor_si128(lo, _mm_slli_si128(mid, 8));
  hi = _mm_xor_si128(hi, _mm_srli_si128(mid, 8));

  /* Single reduction modulo x^128 + x^7 + x^2 + x + 1 */
  __m128i tmp6 = _mm_srli_epi32(lo, 31);
  __m128i tmp7 = _mm_srli_epi32(lo, 30);
  __m128i tmp8 = _mm_srli_epi32(lo, 25);
  tmp6 = _mm_xor_si128(tmp6, tmp7);
  tmp6 = _mm_xor_si128(tmp6, tmp8);
  tmp7 = _mm_shuffle_epi32(tmp6, 0x93);
  tmp6 = _mm_and_si128(tmp7, _mm_set_epi32(0, ~0, ~0, ~0));
  tmp7 = _mm_and_si128(tmp7, _mm_set_epi32(~0, 0, 0, 0));
  lo = _mm_xor_si128(lo, tmp6);
  hi = _mm_xor_si128(hi, tmp7);

  __m128i tmp9 = _mm_slli_epi32(lo, 1);
  lo = _mm_xor_si128(lo, tmp9);
  tmp9 = _mm_slli_epi32(lo, 2);
  lo = _mm_xor_si128(lo, tmp9);
  tmp9 = _mm_slli_epi32(lo, 7);
  lo = _mm_xor_si128(lo, tmp9);
  tmp7 = _mm_srli_si128(lo, 12);
  lo = _mm_slli_si128(lo, 4);
  hi = _mm_xor_si128(hi, lo);
  hi = _mm_xor_si128(hi, tmp7);

  return hi;
}

/**
 * 8-way parallel GHASH with deferred reduction.
 * Accumulates schoolbook partial products across all 8 multiplies,
 * then performs a single Karatsuba combination + single reduction.
 * Saves 7 reductions per 8-block batch vs calling fio___ghash_mult_pclmul 8x.
 *
 * htbl[7] = H⁸ (for x0), htbl[6] = H⁷, ..., htbl[0] = H¹ (for x7).
 */
FIO_IFUNC __m128i fio___x86_ghash_mult8(__m128i x0,
                                        __m128i x1,
                                        __m128i x2,
                                        __m128i x3,
                                        __m128i x4,
                                        __m128i x5,
                                        __m128i x6,
                                        __m128i x7,
                                        const __m128i *htbl) {
  /* Block 0: x0 * H^8 — initialize accumulators */
  __m128i lo = _mm_clmulepi64_si128(x0, htbl[7], 0x00);
  __m128i hi = _mm_clmulepi64_si128(x0, htbl[7], 0x11);
  __m128i m1 = _mm_clmulepi64_si128(x0, htbl[7], 0x01);
  __m128i m2 = _mm_clmulepi64_si128(x0, htbl[7], 0x10);

  /* Block 1: x1 * H^7 — accumulate via XOR */
  lo = _mm_xor_si128(lo, _mm_clmulepi64_si128(x1, htbl[6], 0x00));
  hi = _mm_xor_si128(hi, _mm_clmulepi64_si128(x1, htbl[6], 0x11));
  m1 = _mm_xor_si128(m1, _mm_clmulepi64_si128(x1, htbl[6], 0x01));
  m2 = _mm_xor_si128(m2, _mm_clmulepi64_si128(x1, htbl[6], 0x10));

  /* Block 2: x2 * H^6 */
  lo = _mm_xor_si128(lo, _mm_clmulepi64_si128(x2, htbl[5], 0x00));
  hi = _mm_xor_si128(hi, _mm_clmulepi64_si128(x2, htbl[5], 0x11));
  m1 = _mm_xor_si128(m1, _mm_clmulepi64_si128(x2, htbl[5], 0x01));
  m2 = _mm_xor_si128(m2, _mm_clmulepi64_si128(x2, htbl[5], 0x10));

  /* Block 3: x3 * H^5 */
  lo = _mm_xor_si128(lo, _mm_clmulepi64_si128(x3, htbl[4], 0x00));
  hi = _mm_xor_si128(hi, _mm_clmulepi64_si128(x3, htbl[4], 0x11));
  m1 = _mm_xor_si128(m1, _mm_clmulepi64_si128(x3, htbl[4], 0x01));
  m2 = _mm_xor_si128(m2, _mm_clmulepi64_si128(x3, htbl[4], 0x10));

  /* Block 4: x4 * H^4 */
  lo = _mm_xor_si128(lo, _mm_clmulepi64_si128(x4, htbl[3], 0x00));
  hi = _mm_xor_si128(hi, _mm_clmulepi64_si128(x4, htbl[3], 0x11));
  m1 = _mm_xor_si128(m1, _mm_clmulepi64_si128(x4, htbl[3], 0x01));
  m2 = _mm_xor_si128(m2, _mm_clmulepi64_si128(x4, htbl[3], 0x10));

  /* Block 5: x5 * H^3 */
  lo = _mm_xor_si128(lo, _mm_clmulepi64_si128(x5, htbl[2], 0x00));
  hi = _mm_xor_si128(hi, _mm_clmulepi64_si128(x5, htbl[2], 0x11));
  m1 = _mm_xor_si128(m1, _mm_clmulepi64_si128(x5, htbl[2], 0x01));
  m2 = _mm_xor_si128(m2, _mm_clmulepi64_si128(x5, htbl[2], 0x10));

  /* Block 6: x6 * H^2 */
  lo = _mm_xor_si128(lo, _mm_clmulepi64_si128(x6, htbl[1], 0x00));
  hi = _mm_xor_si128(hi, _mm_clmulepi64_si128(x6, htbl[1], 0x11));
  m1 = _mm_xor_si128(m1, _mm_clmulepi64_si128(x6, htbl[1], 0x01));
  m2 = _mm_xor_si128(m2, _mm_clmulepi64_si128(x6, htbl[1], 0x10));

  /* Block 7: x7 * H^1 */
  lo = _mm_xor_si128(lo, _mm_clmulepi64_si128(x7, htbl[0], 0x00));
  hi = _mm_xor_si128(hi, _mm_clmulepi64_si128(x7, htbl[0], 0x11));
  m1 = _mm_xor_si128(m1, _mm_clmulepi64_si128(x7, htbl[0], 0x01));
  m2 = _mm_xor_si128(m2, _mm_clmulepi64_si128(x7, htbl[0], 0x10));

  /* Single Karatsuba combination on accumulated sums */
  __m128i mid = _mm_xor_si128(m1, m2);
  lo = _mm_xor_si128(lo, _mm_slli_si128(mid, 8));
  hi = _mm_xor_si128(hi, _mm_srli_si128(mid, 8));

  /* Single reduction modulo x^128 + x^7 + x^2 + x + 1 */
  __m128i tmp6 = _mm_srli_epi32(lo, 31);
  __m128i tmp7 = _mm_srli_epi32(lo, 30);
  __m128i tmp8 = _mm_srli_epi32(lo, 25);
  tmp6 = _mm_xor_si128(tmp6, tmp7);
  tmp6 = _mm_xor_si128(tmp6, tmp8);
  tmp7 = _mm_shuffle_epi32(tmp6, 0x93);
  tmp6 = _mm_and_si128(tmp7, _mm_set_epi32(0, ~0, ~0, ~0));
  tmp7 = _mm_and_si128(tmp7, _mm_set_epi32(~0, 0, 0, 0));
  lo = _mm_xor_si128(lo, tmp6);
  hi = _mm_xor_si128(hi, tmp7);

  __m128i tmp9 = _mm_slli_epi32(lo, 1);
  lo = _mm_xor_si128(lo, tmp9);
  tmp9 = _mm_slli_epi32(lo, 2);
  lo = _mm_xor_si128(lo, tmp9);
  tmp9 = _mm_slli_epi32(lo, 7);
  lo = _mm_xor_si128(lo, tmp9);
  tmp7 = _mm_srli_si128(lo, 12);
  lo = _mm_slli_si128(lo, 4);
  hi = _mm_xor_si128(hi, lo);
  hi = _mm_xor_si128(hi, tmp7);

  return hi;
}

/* === Interleaved AES+GHASH macros for 8-block pipeline ===
 *
 * These macros perform AES rounds on 8 blocks while interleaving GHASH
 * schoolbook partial product accumulations. The CPU's out-of-order engine
 * overlaps the independent AES-NI and PCLMULQDQ instruction chains.
 *
 * AES round on 8 blocks (AESENC = AddRoundKey+SubBytes+ShiftRows+MixColumns):
 */
#define FIO___X86_AES_ROUND8(c0, c1, c2, c3, c4, c5, c6, c7, rk_i)             \
  do {                                                                         \
    c0 = _mm_aesenc_si128(c0, rk_i);                                           \
    c1 = _mm_aesenc_si128(c1, rk_i);                                           \
    c2 = _mm_aesenc_si128(c2, rk_i);                                           \
    c3 = _mm_aesenc_si128(c3, rk_i);                                           \
    c4 = _mm_aesenc_si128(c4, rk_i);                                           \
    c5 = _mm_aesenc_si128(c5, rk_i);                                           \
    c6 = _mm_aesenc_si128(c6, rk_i);                                           \
    c7 = _mm_aesenc_si128(c7, rk_i);                                           \
  } while (0)

/* Final AES round (AESENCLAST = no MixColumns) */
#define FIO___X86_AES_LAST8(c0, c1, c2, c3, c4, c5, c6, c7, rk_last)           \
  do {                                                                         \
    c0 = _mm_aesenclast_si128(c0, rk_last);                                    \
    c1 = _mm_aesenclast_si128(c1, rk_last);                                    \
    c2 = _mm_aesenclast_si128(c2, rk_last);                                    \
    c3 = _mm_aesenclast_si128(c3, rk_last);                                    \
    c4 = _mm_aesenclast_si128(c4, rk_last);                                    \
    c5 = _mm_aesenclast_si128(c5, rk_last);                                    \
    c6 = _mm_aesenclast_si128(c6, rk_last);                                    \
    c7 = _mm_aesenclast_si128(c7, rk_last);                                    \
  } while (0)

/* GHASH schoolbook partial product for one block — initialize accumulators.
 * This is the first block of an 8-block batch. */
#define FIO___X86_GHASH_INIT(gh_lo, gh_hi, gh_m1, gh_m2, x, h)                 \
  do {                                                                         \
    gh_lo = _mm_clmulepi64_si128(x, h, 0x00);                                  \
    gh_hi = _mm_clmulepi64_si128(x, h, 0x11);                                  \
    gh_m1 = _mm_clmulepi64_si128(x, h, 0x01);                                  \
    gh_m2 = _mm_clmulepi64_si128(x, h, 0x10);                                  \
  } while (0)

/* GHASH schoolbook partial product — accumulate into existing accumulators */
#define FIO___X86_GHASH_ACCUM(gh_lo, gh_hi, gh_m1, gh_m2, x, h)                \
  do {                                                                         \
    gh_lo = _mm_xor_si128(gh_lo, _mm_clmulepi64_si128(x, h, 0x00));            \
    gh_hi = _mm_xor_si128(gh_hi, _mm_clmulepi64_si128(x, h, 0x11));            \
    gh_m1 = _mm_xor_si128(gh_m1, _mm_clmulepi64_si128(x, h, 0x01));            \
    gh_m2 = _mm_xor_si128(gh_m2, _mm_clmulepi64_si128(x, h, 0x10));            \
  } while (0)

/* Finalize GHASH: Karatsuba combination + reduction.
 * Produces the final 128-bit GHASH result from accumulated partial products. */
#define FIO___X86_GHASH_FINAL(result, gh_lo, gh_hi, gh_m1, gh_m2)              \
  do {                                                                         \
    __m128i gf_mid_ = _mm_xor_si128(gh_m1, gh_m2);                             \
    gh_lo = _mm_xor_si128(gh_lo, _mm_slli_si128(gf_mid_, 8));                  \
    gh_hi = _mm_xor_si128(gh_hi, _mm_srli_si128(gf_mid_, 8));                  \
    /* Reduction modulo x^128 + x^7 + x^2 + x + 1 */                           \
    __m128i gf6_ = _mm_srli_epi32(gh_lo, 31);                                  \
    __m128i gf7_ = _mm_srli_epi32(gh_lo, 30);                                  \
    __m128i gf8_ = _mm_srli_epi32(gh_lo, 25);                                  \
    gf6_ = _mm_xor_si128(gf6_, gf7_);                                          \
    gf6_ = _mm_xor_si128(gf6_, gf8_);                                          \
    gf7_ = _mm_shuffle_epi32(gf6_, 0x93);                                      \
    gf6_ = _mm_and_si128(gf7_, _mm_set_epi32(0, ~0, ~0, ~0));                  \
    gf7_ = _mm_and_si128(gf7_, _mm_set_epi32(~0, 0, 0, 0));                    \
    gh_lo = _mm_xor_si128(gh_lo, gf6_);                                        \
    gh_hi = _mm_xor_si128(gh_hi, gf7_);                                        \
    __m128i gf9_ = _mm_slli_epi32(gh_lo, 1);                                   \
    gh_lo = _mm_xor_si128(gh_lo, gf9_);                                        \
    gf9_ = _mm_slli_epi32(gh_lo, 2);                                           \
    gh_lo = _mm_xor_si128(gh_lo, gf9_);                                        \
    gf9_ = _mm_slli_epi32(gh_lo, 7);                                           \
    gh_lo = _mm_xor_si128(gh_lo, gf9_);                                        \
    gf7_ = _mm_srli_si128(gh_lo, 12);                                          \
    gh_lo = _mm_slli_si128(gh_lo, 4);                                          \
    gh_hi = _mm_xor_si128(gh_hi, gh_lo);                                       \
    result = _mm_xor_si128(gh_hi, gf7_);                                       \
  } while (0)

/* Increment counter (last 32 bits, big-endian) */
FIO_IFUNC __m128i fio___gcm_inc_ctr(__m128i ctr) {
  /* Counter is in bytes 12-15 (big-endian) */
  __m128i one = _mm_set_epi32(1, 0, 0, 0);
  ctr = fio___bswap128(ctr);
  ctr = _mm_add_epi32(ctr, one);
  return fio___bswap128(ctr);
}

SFUNC void fio_aes128_gcm_enc(void *restrict mac,
                              void *restrict data,
                              size_t len,
                              const void *ad,
                              size_t adlen,
                              const void *key,
                              const void *nonce) {
  __m128i rk[11];
  __m128i h, htbl[8], tag, ctr, j0;
  uint8_t *p = (uint8_t *)data;
  const uint8_t *aad = (const uint8_t *)ad;
  size_t orig_len = len;
  size_t orig_adlen = adlen;

  fio___aesni_gcm128_init(rk, &h, (const uint8_t *)key);
  h = fio___bswap128(h);

  /* Precompute H powers: H⁸ for 8-block, H⁴ for 4-block, H¹ for small */
  if (len >= 128 || adlen >= 128)
    fio___x86_ghash_precompute(h, htbl, 1);
  else if (len >= 64 || adlen >= 64)
    fio___x86_ghash_precompute(h, htbl, 0);
  else
    htbl[0] = h; /* Only H^1 needed for single-block path */

  uint8_t j0_bytes[16] = {0};
  FIO_MEMCPY(j0_bytes, nonce, 12);
  j0_bytes[15] = 1;
  j0 = _mm_loadu_si128((const __m128i *)j0_bytes);
  ctr = j0;
  tag = _mm_setzero_si128();

  /* GHASH over AAD - process 8 blocks, then 4 blocks, then single */
  while (adlen >= 128) {
    __m128i a0 =
        _mm_xor_si128(tag,
                      fio___bswap128(_mm_loadu_si128((const __m128i *)aad)));
    __m128i a1 = fio___bswap128(_mm_loadu_si128((const __m128i *)(aad + 16)));
    __m128i a2 = fio___bswap128(_mm_loadu_si128((const __m128i *)(aad + 32)));
    __m128i a3 = fio___bswap128(_mm_loadu_si128((const __m128i *)(aad + 48)));
    __m128i a4 = fio___bswap128(_mm_loadu_si128((const __m128i *)(aad + 64)));
    __m128i a5 = fio___bswap128(_mm_loadu_si128((const __m128i *)(aad + 80)));
    __m128i a6 = fio___bswap128(_mm_loadu_si128((const __m128i *)(aad + 96)));
    __m128i a7 = fio___bswap128(_mm_loadu_si128((const __m128i *)(aad + 112)));
    tag = fio___x86_ghash_mult8(a0, a1, a2, a3, a4, a5, a6, a7, htbl);
    aad += 128;
    adlen -= 128;
  }
  while (adlen >= 64) {
    __m128i a0 = fio___bswap128(_mm_loadu_si128((const __m128i *)aad));
    __m128i a1 = fio___bswap128(_mm_loadu_si128((const __m128i *)(aad + 16)));
    __m128i a2 = fio___bswap128(_mm_loadu_si128((const __m128i *)(aad + 32)));
    __m128i a3 = fio___bswap128(_mm_loadu_si128((const __m128i *)(aad + 48)));
    a0 = _mm_xor_si128(tag, a0);
    tag = fio___x86_ghash_mult4(a0, a1, a2, a3, htbl);
    aad += 64;
    adlen -= 64;
  }
  while (adlen >= 16) {
    __m128i aad_block = _mm_loadu_si128((const __m128i *)aad);
    aad_block = fio___bswap128(aad_block);
    tag = _mm_xor_si128(tag, aad_block);
    tag = fio___ghash_mult_pclmul(tag, h);
    aad += 16;
    adlen -= 16;
  }
  if (adlen > 0) {
    uint8_t tmp[16] = {0};
    FIO_MEMCPY(tmp, aad, adlen);
    __m128i aad_block = _mm_loadu_si128((const __m128i *)tmp);
    aad_block = fio___bswap128(aad_block);
    tag = _mm_xor_si128(tag, aad_block);
    tag = fio___ghash_mult_pclmul(tag, h);
  }

  /* === 8-block interleaved AES-CTR encryption + GHASH === */
  if (len >= 256) {
    /* Prologue: encrypt first 8 blocks (no previous ciphertext to GHASH) */
    __m128i ctr0 = fio___gcm_inc_ctr(ctr);
    __m128i ctr1 = fio___gcm_inc_ctr(ctr0);
    __m128i ctr2 = fio___gcm_inc_ctr(ctr1);
    __m128i ctr3 = fio___gcm_inc_ctr(ctr2);
    __m128i ctr4 = fio___gcm_inc_ctr(ctr3);
    __m128i ctr5 = fio___gcm_inc_ctr(ctr4);
    __m128i ctr6 = fio___gcm_inc_ctr(ctr5);
    __m128i ctr7 = fio___gcm_inc_ctr(ctr6);
    ctr = ctr7;

    __m128i ct0 = _mm_xor_si128(_mm_loadu_si128((const __m128i *)p),
                                fio___aesni_encrypt128(ctr0, rk));
    __m128i ct1 = _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 16)),
                                fio___aesni_encrypt128(ctr1, rk));
    __m128i ct2 = _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 32)),
                                fio___aesni_encrypt128(ctr2, rk));
    __m128i ct3 = _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 48)),
                                fio___aesni_encrypt128(ctr3, rk));
    __m128i ct4 = _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 64)),
                                fio___aesni_encrypt128(ctr4, rk));
    __m128i ct5 = _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 80)),
                                fio___aesni_encrypt128(ctr5, rk));
    __m128i ct6 = _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 96)),
                                fio___aesni_encrypt128(ctr6, rk));
    __m128i ct7 = _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 112)),
                                fio___aesni_encrypt128(ctr7, rk));

    _mm_storeu_si128((__m128i *)p, ct0);
    _mm_storeu_si128((__m128i *)(p + 16), ct1);
    _mm_storeu_si128((__m128i *)(p + 32), ct2);
    _mm_storeu_si128((__m128i *)(p + 48), ct3);
    _mm_storeu_si128((__m128i *)(p + 64), ct4);
    _mm_storeu_si128((__m128i *)(p + 80), ct5);
    _mm_storeu_si128((__m128i *)(p + 96), ct6);
    _mm_storeu_si128((__m128i *)(p + 112), ct7);

    /* Save previous ciphertext (byte-swapped) for GHASH in next iteration */
    __m128i prev0 = _mm_xor_si128(tag, fio___bswap128(ct0));
    __m128i prev1 = fio___bswap128(ct1);
    __m128i prev2 = fio___bswap128(ct2);
    __m128i prev3 = fio___bswap128(ct3);
    __m128i prev4 = fio___bswap128(ct4);
    __m128i prev5 = fio___bswap128(ct5);
    __m128i prev6 = fio___bswap128(ct6);
    __m128i prev7 = fio___bswap128(ct7);

    p += 128;
    len -= 128;

    /* Steady state: interleaved AES-128 encrypt + GHASH.
     * AES rounds and GHASH schoolbook products are interleaved at the macro
     * level so the CPU's OoO engine overlaps the independent chains.
     * AES-128 has 10 rounds: 1 initial XOR + 9 AESENC + 1 AESENCLAST.
     * We interleave 8 GHASH blocks across the 9 AESENC rounds. */
    while (len >= 128) {
      __m128i gh_lo, gh_hi, gh_m1, gh_m2;

      /* Prepare 8 new counter blocks */
      ctr0 = fio___gcm_inc_ctr(ctr);
      ctr1 = fio___gcm_inc_ctr(ctr0);
      ctr2 = fio___gcm_inc_ctr(ctr1);
      ctr3 = fio___gcm_inc_ctr(ctr2);
      ctr4 = fio___gcm_inc_ctr(ctr3);
      ctr5 = fio___gcm_inc_ctr(ctr4);
      ctr6 = fio___gcm_inc_ctr(ctr5);
      ctr7 = fio___gcm_inc_ctr(ctr6);
      ctr = ctr7;

      /* Initial AddRoundKey */
      ctr0 = _mm_xor_si128(ctr0, rk[0]);
      ctr1 = _mm_xor_si128(ctr1, rk[0]);
      ctr2 = _mm_xor_si128(ctr2, rk[0]);
      ctr3 = _mm_xor_si128(ctr3, rk[0]);
      ctr4 = _mm_xor_si128(ctr4, rk[0]);
      ctr5 = _mm_xor_si128(ctr5, rk[0]);
      ctr6 = _mm_xor_si128(ctr6, rk[0]);
      ctr7 = _mm_xor_si128(ctr7, rk[0]);

      /* AES round 1 + GHASH block 0 (init) */
      FIO___X86_AES_ROUND8(ctr0,
                           ctr1,
                           ctr2,
                           ctr3,
                           ctr4,
                           ctr5,
                           ctr6,
                           ctr7,
                           rk[1]);
      FIO___X86_GHASH_INIT(gh_lo, gh_hi, gh_m1, gh_m2, prev0, htbl[7]);

      /* AES round 2 + GHASH block 1 */
      FIO___X86_AES_ROUND8(ctr0,
                           ctr1,
                           ctr2,
                           ctr3,
                           ctr4,
                           ctr5,
                           ctr6,
                           ctr7,
                           rk[2]);
      FIO___X86_GHASH_ACCUM(gh_lo, gh_hi, gh_m1, gh_m2, prev1, htbl[6]);

      /* AES round 3 + GHASH block 2 */
      FIO___X86_AES_ROUND8(ctr0,
                           ctr1,
                           ctr2,
                           ctr3,
                           ctr4,
                           ctr5,
                           ctr6,
                           ctr7,
                           rk[3]);
      FIO___X86_GHASH_ACCUM(gh_lo, gh_hi, gh_m1, gh_m2, prev2, htbl[5]);

      /* AES round 4 + GHASH block 3 */
      FIO___X86_AES_ROUND8(ctr0,
                           ctr1,
                           ctr2,
                           ctr3,
                           ctr4,
                           ctr5,
                           ctr6,
                           ctr7,
                           rk[4]);
      FIO___X86_GHASH_ACCUM(gh_lo, gh_hi, gh_m1, gh_m2, prev3, htbl[4]);

      /* AES round 5 + GHASH block 4 */
      FIO___X86_AES_ROUND8(ctr0,
                           ctr1,
                           ctr2,
                           ctr3,
                           ctr4,
                           ctr5,
                           ctr6,
                           ctr7,
                           rk[5]);
      FIO___X86_GHASH_ACCUM(gh_lo, gh_hi, gh_m1, gh_m2, prev4, htbl[3]);

      /* AES round 6 + GHASH block 5 */
      FIO___X86_AES_ROUND8(ctr0,
                           ctr1,
                           ctr2,
                           ctr3,
                           ctr4,
                           ctr5,
                           ctr6,
                           ctr7,
                           rk[6]);
      FIO___X86_GHASH_ACCUM(gh_lo, gh_hi, gh_m1, gh_m2, prev5, htbl[2]);

      /* AES round 7 + GHASH block 6 */
      FIO___X86_AES_ROUND8(ctr0,
                           ctr1,
                           ctr2,
                           ctr3,
                           ctr4,
                           ctr5,
                           ctr6,
                           ctr7,
                           rk[7]);
      FIO___X86_GHASH_ACCUM(gh_lo, gh_hi, gh_m1, gh_m2, prev6, htbl[1]);

      /* AES round 8 + GHASH block 7 */
      FIO___X86_AES_ROUND8(ctr0,
                           ctr1,
                           ctr2,
                           ctr3,
                           ctr4,
                           ctr5,
                           ctr6,
                           ctr7,
                           rk[8]);
      FIO___X86_GHASH_ACCUM(gh_lo, gh_hi, gh_m1, gh_m2, prev7, htbl[0]);

      /* AES round 9 + GHASH finalize */
      FIO___X86_AES_ROUND8(ctr0,
                           ctr1,
                           ctr2,
                           ctr3,
                           ctr4,
                           ctr5,
                           ctr6,
                           ctr7,
                           rk[9]);
      FIO___X86_GHASH_FINAL(tag, gh_lo, gh_hi, gh_m1, gh_m2);

      /* AES final round */
      FIO___X86_AES_LAST8(ctr0,
                          ctr1,
                          ctr2,
                          ctr3,
                          ctr4,
                          ctr5,
                          ctr6,
                          ctr7,
                          rk[10]);

      /* XOR keystream with plaintext → ciphertext */
      ct0 = _mm_xor_si128(_mm_loadu_si128((const __m128i *)p), ctr0);
      ct1 = _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 16)), ctr1);
      ct2 = _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 32)), ctr2);
      ct3 = _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 48)), ctr3);
      ct4 = _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 64)), ctr4);
      ct5 = _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 80)), ctr5);
      ct6 = _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 96)), ctr6);
      ct7 = _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 112)), ctr7);

      _mm_storeu_si128((__m128i *)p, ct0);
      _mm_storeu_si128((__m128i *)(p + 16), ct1);
      _mm_storeu_si128((__m128i *)(p + 32), ct2);
      _mm_storeu_si128((__m128i *)(p + 48), ct3);
      _mm_storeu_si128((__m128i *)(p + 64), ct4);
      _mm_storeu_si128((__m128i *)(p + 80), ct5);
      _mm_storeu_si128((__m128i *)(p + 96), ct6);
      _mm_storeu_si128((__m128i *)(p + 112), ct7);

      /* Save current ciphertext for GHASH in next iteration */
      prev0 = _mm_xor_si128(tag, fio___bswap128(ct0));
      prev1 = fio___bswap128(ct1);
      prev2 = fio___bswap128(ct2);
      prev3 = fio___bswap128(ct3);
      prev4 = fio___bswap128(ct4);
      prev5 = fio___bswap128(ct5);
      prev6 = fio___bswap128(ct6);
      prev7 = fio___bswap128(ct7);

      p += 128;
      len -= 128;
    }

    /* Epilogue: GHASH the last 8-block batch */
    tag = fio___x86_ghash_mult8(prev0,
                                prev1,
                                prev2,
                                prev3,
                                prev4,
                                prev5,
                                prev6,
                                prev7,
                                htbl);
  }

  /* 4-block tail */
  while (len >= 64) {
    __m128i ctr0 = fio___gcm_inc_ctr(ctr);
    __m128i ctr1 = fio___gcm_inc_ctr(ctr0);
    __m128i ctr2 = fio___gcm_inc_ctr(ctr1);
    __m128i ctr3 = fio___gcm_inc_ctr(ctr2);
    ctr = ctr3;

    __m128i ks0 = fio___aesni_encrypt128(ctr0, rk);
    __m128i ks1 = fio___aesni_encrypt128(ctr1, rk);
    __m128i ks2 = fio___aesni_encrypt128(ctr2, rk);
    __m128i ks3 = fio___aesni_encrypt128(ctr3, rk);

    __m128i ct0 = _mm_xor_si128(_mm_loadu_si128((const __m128i *)p), ks0);
    __m128i ct1 =
        _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 16)), ks1);
    __m128i ct2 =
        _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 32)), ks2);
    __m128i ct3 =
        _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 48)), ks3);

    _mm_storeu_si128((__m128i *)p, ct0);
    _mm_storeu_si128((__m128i *)(p + 16), ct1);
    _mm_storeu_si128((__m128i *)(p + 32), ct2);
    _mm_storeu_si128((__m128i *)(p + 48), ct3);

    ct0 = _mm_xor_si128(tag, fio___bswap128(ct0));
    tag = fio___x86_ghash_mult4(ct0,
                                fio___bswap128(ct1),
                                fio___bswap128(ct2),
                                fio___bswap128(ct3),
                                htbl);
    p += 64;
    len -= 64;
  }

  /* Single-block tail */
  while (len >= 16) {
    ctr = fio___gcm_inc_ctr(ctr);
    __m128i keystream = fio___aesni_encrypt128(ctr, rk);
    __m128i plaintext = _mm_loadu_si128((const __m128i *)p);
    __m128i ciphertext = _mm_xor_si128(plaintext, keystream);
    _mm_storeu_si128((__m128i *)p, ciphertext);
    ciphertext = fio___bswap128(ciphertext);
    tag = _mm_xor_si128(tag, ciphertext);
    tag = fio___ghash_mult_pclmul(tag, h);
    p += 16;
    len -= 16;
  }

  /* Handle partial final block */
  if (len > 0) {
    ctr = fio___gcm_inc_ctr(ctr);
    __m128i keystream = fio___aesni_encrypt128(ctr, rk);
    uint8_t ks_bytes[16];
    _mm_storeu_si128((__m128i *)ks_bytes, keystream);
    for (size_t i = 0; i < len; ++i)
      p[i] ^= ks_bytes[i];
    uint8_t tmp[16] = {0};
    FIO_MEMCPY(tmp, p, len);
    __m128i ct_block = _mm_loadu_si128((const __m128i *)tmp);
    ct_block = fio___bswap128(ct_block);
    tag = _mm_xor_si128(tag, ct_block);
    tag = fio___ghash_mult_pclmul(tag, h);
  }

  /* GHASH length block */
  uint8_t len_block[16] = {0};
  fio_u2buf64_be(len_block, (uint64_t)orig_adlen * 8);
  fio_u2buf64_be(len_block + 8, (uint64_t)orig_len * 8);
  __m128i len_blk = _mm_loadu_si128((const __m128i *)len_block);
  len_blk = fio___bswap128(len_blk);
  tag = _mm_xor_si128(tag, len_blk);
  tag = fio___ghash_mult_pclmul(tag, h);

  /* Final tag */
  __m128i s = fio___aesni_encrypt128(j0, rk);
  tag = fio___bswap128(tag);
  tag = _mm_xor_si128(tag, s);
  _mm_storeu_si128((__m128i *)mac, tag);
  /* Clear sensitive data */
  fio_secure_zero(rk, sizeof(rk));
  fio_secure_zero(htbl, sizeof(htbl));
  fio_secure_zero(j0_bytes, sizeof(j0_bytes));
}

SFUNC void fio_aes256_gcm_enc(void *restrict mac,
                              void *restrict data,
                              size_t len,
                              const void *ad,
                              size_t adlen,
                              const void *key,
                              const void *nonce) {
  __m128i rk[15];
  __m128i h, htbl[8], tag, ctr, j0;
  uint8_t *p = (uint8_t *)data;
  const uint8_t *aad = (const uint8_t *)ad;
  size_t orig_len = len;
  size_t orig_adlen = adlen;

  fio___aesni_gcm256_init(rk, &h, (const uint8_t *)key);
  h = fio___bswap128(h);

  /* Precompute H powers: H⁸ for 8-block, H⁴ for 4-block, H¹ for small */
  if (len >= 128 || adlen >= 128)
    fio___x86_ghash_precompute(h, htbl, 1);
  else if (len >= 64 || adlen >= 64)
    fio___x86_ghash_precompute(h, htbl, 0);
  else
    htbl[0] = h; /* Only H^1 needed for single-block path */

  uint8_t j0_bytes[16] = {0};
  FIO_MEMCPY(j0_bytes, nonce, 12);
  j0_bytes[15] = 1;
  j0 = _mm_loadu_si128((const __m128i *)j0_bytes);
  ctr = j0;
  tag = _mm_setzero_si128();

  /* GHASH over AAD - 8-block, 4-block, single */
  while (adlen >= 128) {
    __m128i a0 =
        _mm_xor_si128(tag,
                      fio___bswap128(_mm_loadu_si128((const __m128i *)aad)));
    __m128i a1 = fio___bswap128(_mm_loadu_si128((const __m128i *)(aad + 16)));
    __m128i a2 = fio___bswap128(_mm_loadu_si128((const __m128i *)(aad + 32)));
    __m128i a3 = fio___bswap128(_mm_loadu_si128((const __m128i *)(aad + 48)));
    __m128i a4 = fio___bswap128(_mm_loadu_si128((const __m128i *)(aad + 64)));
    __m128i a5 = fio___bswap128(_mm_loadu_si128((const __m128i *)(aad + 80)));
    __m128i a6 = fio___bswap128(_mm_loadu_si128((const __m128i *)(aad + 96)));
    __m128i a7 = fio___bswap128(_mm_loadu_si128((const __m128i *)(aad + 112)));
    tag = fio___x86_ghash_mult8(a0, a1, a2, a3, a4, a5, a6, a7, htbl);
    aad += 128;
    adlen -= 128;
  }
  while (adlen >= 64) {
    __m128i a0 = fio___bswap128(_mm_loadu_si128((const __m128i *)aad));
    __m128i a1 = fio___bswap128(_mm_loadu_si128((const __m128i *)(aad + 16)));
    __m128i a2 = fio___bswap128(_mm_loadu_si128((const __m128i *)(aad + 32)));
    __m128i a3 = fio___bswap128(_mm_loadu_si128((const __m128i *)(aad + 48)));
    a0 = _mm_xor_si128(tag, a0);
    tag = fio___x86_ghash_mult4(a0, a1, a2, a3, htbl);
    aad += 64;
    adlen -= 64;
  }
  while (adlen >= 16) {
    __m128i aad_block = _mm_loadu_si128((const __m128i *)aad);
    aad_block = fio___bswap128(aad_block);
    tag = _mm_xor_si128(tag, aad_block);
    tag = fio___ghash_mult_pclmul(tag, h);
    aad += 16;
    adlen -= 16;
  }
  if (adlen > 0) {
    uint8_t tmp[16] = {0};
    FIO_MEMCPY(tmp, aad, adlen);
    __m128i aad_block = _mm_loadu_si128((const __m128i *)tmp);
    aad_block = fio___bswap128(aad_block);
    tag = _mm_xor_si128(tag, aad_block);
    tag = fio___ghash_mult_pclmul(tag, h);
  }

  /* === 8-block interleaved AES-CTR encryption + GHASH === */
  if (len >= 256) {
    /* Prologue: encrypt first 8 blocks */
    __m128i ctr0 = fio___gcm_inc_ctr(ctr);
    __m128i ctr1 = fio___gcm_inc_ctr(ctr0);
    __m128i ctr2 = fio___gcm_inc_ctr(ctr1);
    __m128i ctr3 = fio___gcm_inc_ctr(ctr2);
    __m128i ctr4 = fio___gcm_inc_ctr(ctr3);
    __m128i ctr5 = fio___gcm_inc_ctr(ctr4);
    __m128i ctr6 = fio___gcm_inc_ctr(ctr5);
    __m128i ctr7 = fio___gcm_inc_ctr(ctr6);
    ctr = ctr7;

    __m128i ct0 = _mm_xor_si128(_mm_loadu_si128((const __m128i *)p),
                                fio___aesni_encrypt256(ctr0, rk));
    __m128i ct1 = _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 16)),
                                fio___aesni_encrypt256(ctr1, rk));
    __m128i ct2 = _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 32)),
                                fio___aesni_encrypt256(ctr2, rk));
    __m128i ct3 = _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 48)),
                                fio___aesni_encrypt256(ctr3, rk));
    __m128i ct4 = _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 64)),
                                fio___aesni_encrypt256(ctr4, rk));
    __m128i ct5 = _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 80)),
                                fio___aesni_encrypt256(ctr5, rk));
    __m128i ct6 = _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 96)),
                                fio___aesni_encrypt256(ctr6, rk));
    __m128i ct7 = _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 112)),
                                fio___aesni_encrypt256(ctr7, rk));

    _mm_storeu_si128((__m128i *)p, ct0);
    _mm_storeu_si128((__m128i *)(p + 16), ct1);
    _mm_storeu_si128((__m128i *)(p + 32), ct2);
    _mm_storeu_si128((__m128i *)(p + 48), ct3);
    _mm_storeu_si128((__m128i *)(p + 64), ct4);
    _mm_storeu_si128((__m128i *)(p + 80), ct5);
    _mm_storeu_si128((__m128i *)(p + 96), ct6);
    _mm_storeu_si128((__m128i *)(p + 112), ct7);

    __m128i prev0 = _mm_xor_si128(tag, fio___bswap128(ct0));
    __m128i prev1 = fio___bswap128(ct1);
    __m128i prev2 = fio___bswap128(ct2);
    __m128i prev3 = fio___bswap128(ct3);
    __m128i prev4 = fio___bswap128(ct4);
    __m128i prev5 = fio___bswap128(ct5);
    __m128i prev6 = fio___bswap128(ct6);
    __m128i prev7 = fio___bswap128(ct7);

    p += 128;
    len -= 128;

    /* Steady state: interleaved AES-256 encrypt + GHASH.
     * AES-256 has 14 rounds: 1 initial XOR + 13 AESENC + 1 AESENCLAST.
     * We interleave 8 GHASH blocks across the first 9 AESENC rounds,
     * then run the remaining 4 AESENC rounds + final round. */
    while (len >= 128) {
      __m128i gh_lo, gh_hi, gh_m1, gh_m2;

      ctr0 = fio___gcm_inc_ctr(ctr);
      ctr1 = fio___gcm_inc_ctr(ctr0);
      ctr2 = fio___gcm_inc_ctr(ctr1);
      ctr3 = fio___gcm_inc_ctr(ctr2);
      ctr4 = fio___gcm_inc_ctr(ctr3);
      ctr5 = fio___gcm_inc_ctr(ctr4);
      ctr6 = fio___gcm_inc_ctr(ctr5);
      ctr7 = fio___gcm_inc_ctr(ctr6);
      ctr = ctr7;

      /* Initial AddRoundKey */
      ctr0 = _mm_xor_si128(ctr0, rk[0]);
      ctr1 = _mm_xor_si128(ctr1, rk[0]);
      ctr2 = _mm_xor_si128(ctr2, rk[0]);
      ctr3 = _mm_xor_si128(ctr3, rk[0]);
      ctr4 = _mm_xor_si128(ctr4, rk[0]);
      ctr5 = _mm_xor_si128(ctr5, rk[0]);
      ctr6 = _mm_xor_si128(ctr6, rk[0]);
      ctr7 = _mm_xor_si128(ctr7, rk[0]);

      /* AES rounds 1-8 interleaved with GHASH blocks 0-7 */
      FIO___X86_AES_ROUND8(ctr0,
                           ctr1,
                           ctr2,
                           ctr3,
                           ctr4,
                           ctr5,
                           ctr6,
                           ctr7,
                           rk[1]);
      FIO___X86_GHASH_INIT(gh_lo, gh_hi, gh_m1, gh_m2, prev0, htbl[7]);

      FIO___X86_AES_ROUND8(ctr0,
                           ctr1,
                           ctr2,
                           ctr3,
                           ctr4,
                           ctr5,
                           ctr6,
                           ctr7,
                           rk[2]);
      FIO___X86_GHASH_ACCUM(gh_lo, gh_hi, gh_m1, gh_m2, prev1, htbl[6]);

      FIO___X86_AES_ROUND8(ctr0,
                           ctr1,
                           ctr2,
                           ctr3,
                           ctr4,
                           ctr5,
                           ctr6,
                           ctr7,
                           rk[3]);
      FIO___X86_GHASH_ACCUM(gh_lo, gh_hi, gh_m1, gh_m2, prev2, htbl[5]);

      FIO___X86_AES_ROUND8(ctr0,
                           ctr1,
                           ctr2,
                           ctr3,
                           ctr4,
                           ctr5,
                           ctr6,
                           ctr7,
                           rk[4]);
      FIO___X86_GHASH_ACCUM(gh_lo, gh_hi, gh_m1, gh_m2, prev3, htbl[4]);

      FIO___X86_AES_ROUND8(ctr0,
                           ctr1,
                           ctr2,
                           ctr3,
                           ctr4,
                           ctr5,
                           ctr6,
                           ctr7,
                           rk[5]);
      FIO___X86_GHASH_ACCUM(gh_lo, gh_hi, gh_m1, gh_m2, prev4, htbl[3]);

      FIO___X86_AES_ROUND8(ctr0,
                           ctr1,
                           ctr2,
                           ctr3,
                           ctr4,
                           ctr5,
                           ctr6,
                           ctr7,
                           rk[6]);
      FIO___X86_GHASH_ACCUM(gh_lo, gh_hi, gh_m1, gh_m2, prev5, htbl[2]);

      FIO___X86_AES_ROUND8(ctr0,
                           ctr1,
                           ctr2,
                           ctr3,
                           ctr4,
                           ctr5,
                           ctr6,
                           ctr7,
                           rk[7]);
      FIO___X86_GHASH_ACCUM(gh_lo, gh_hi, gh_m1, gh_m2, prev6, htbl[1]);

      FIO___X86_AES_ROUND8(ctr0,
                           ctr1,
                           ctr2,
                           ctr3,
                           ctr4,
                           ctr5,
                           ctr6,
                           ctr7,
                           rk[8]);
      FIO___X86_GHASH_ACCUM(gh_lo, gh_hi, gh_m1, gh_m2, prev7, htbl[0]);

      /* AES round 9 + GHASH finalize */
      FIO___X86_AES_ROUND8(ctr0,
                           ctr1,
                           ctr2,
                           ctr3,
                           ctr4,
                           ctr5,
                           ctr6,
                           ctr7,
                           rk[9]);
      FIO___X86_GHASH_FINAL(tag, gh_lo, gh_hi, gh_m1, gh_m2);

      /* AES rounds 10-13 + final round */
      FIO___X86_AES_ROUND8(ctr0,
                           ctr1,
                           ctr2,
                           ctr3,
                           ctr4,
                           ctr5,
                           ctr6,
                           ctr7,
                           rk[10]);
      FIO___X86_AES_ROUND8(ctr0,
                           ctr1,
                           ctr2,
                           ctr3,
                           ctr4,
                           ctr5,
                           ctr6,
                           ctr7,
                           rk[11]);
      FIO___X86_AES_ROUND8(ctr0,
                           ctr1,
                           ctr2,
                           ctr3,
                           ctr4,
                           ctr5,
                           ctr6,
                           ctr7,
                           rk[12]);
      FIO___X86_AES_ROUND8(ctr0,
                           ctr1,
                           ctr2,
                           ctr3,
                           ctr4,
                           ctr5,
                           ctr6,
                           ctr7,
                           rk[13]);
      FIO___X86_AES_LAST8(ctr0,
                          ctr1,
                          ctr2,
                          ctr3,
                          ctr4,
                          ctr5,
                          ctr6,
                          ctr7,
                          rk[14]);

      ct0 = _mm_xor_si128(_mm_loadu_si128((const __m128i *)p), ctr0);
      ct1 = _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 16)), ctr1);
      ct2 = _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 32)), ctr2);
      ct3 = _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 48)), ctr3);
      ct4 = _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 64)), ctr4);
      ct5 = _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 80)), ctr5);
      ct6 = _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 96)), ctr6);
      ct7 = _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 112)), ctr7);

      _mm_storeu_si128((__m128i *)p, ct0);
      _mm_storeu_si128((__m128i *)(p + 16), ct1);
      _mm_storeu_si128((__m128i *)(p + 32), ct2);
      _mm_storeu_si128((__m128i *)(p + 48), ct3);
      _mm_storeu_si128((__m128i *)(p + 64), ct4);
      _mm_storeu_si128((__m128i *)(p + 80), ct5);
      _mm_storeu_si128((__m128i *)(p + 96), ct6);
      _mm_storeu_si128((__m128i *)(p + 112), ct7);

      prev0 = _mm_xor_si128(tag, fio___bswap128(ct0));
      prev1 = fio___bswap128(ct1);
      prev2 = fio___bswap128(ct2);
      prev3 = fio___bswap128(ct3);
      prev4 = fio___bswap128(ct4);
      prev5 = fio___bswap128(ct5);
      prev6 = fio___bswap128(ct6);
      prev7 = fio___bswap128(ct7);

      p += 128;
      len -= 128;
    }

    /* Epilogue: GHASH the last 8-block batch */
    tag = fio___x86_ghash_mult8(prev0,
                                prev1,
                                prev2,
                                prev3,
                                prev4,
                                prev5,
                                prev6,
                                prev7,
                                htbl);
  }

  /* 4-block tail */
  while (len >= 64) {
    __m128i ctr0 = fio___gcm_inc_ctr(ctr);
    __m128i ctr1 = fio___gcm_inc_ctr(ctr0);
    __m128i ctr2 = fio___gcm_inc_ctr(ctr1);
    __m128i ctr3 = fio___gcm_inc_ctr(ctr2);
    ctr = ctr3;

    __m128i ks0 = fio___aesni_encrypt256(ctr0, rk);
    __m128i ks1 = fio___aesni_encrypt256(ctr1, rk);
    __m128i ks2 = fio___aesni_encrypt256(ctr2, rk);
    __m128i ks3 = fio___aesni_encrypt256(ctr3, rk);

    __m128i ct0 = _mm_xor_si128(_mm_loadu_si128((const __m128i *)p), ks0);
    __m128i ct1 =
        _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 16)), ks1);
    __m128i ct2 =
        _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 32)), ks2);
    __m128i ct3 =
        _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 48)), ks3);

    _mm_storeu_si128((__m128i *)p, ct0);
    _mm_storeu_si128((__m128i *)(p + 16), ct1);
    _mm_storeu_si128((__m128i *)(p + 32), ct2);
    _mm_storeu_si128((__m128i *)(p + 48), ct3);

    ct0 = _mm_xor_si128(tag, fio___bswap128(ct0));
    tag = fio___x86_ghash_mult4(ct0,
                                fio___bswap128(ct1),
                                fio___bswap128(ct2),
                                fio___bswap128(ct3),
                                htbl);
    p += 64;
    len -= 64;
  }

  /* Single-block tail */
  while (len >= 16) {
    ctr = fio___gcm_inc_ctr(ctr);
    __m128i keystream = fio___aesni_encrypt256(ctr, rk);
    __m128i plaintext = _mm_loadu_si128((const __m128i *)p);
    __m128i ciphertext = _mm_xor_si128(plaintext, keystream);
    _mm_storeu_si128((__m128i *)p, ciphertext);
    ciphertext = fio___bswap128(ciphertext);
    tag = _mm_xor_si128(tag, ciphertext);
    tag = fio___ghash_mult_pclmul(tag, h);
    p += 16;
    len -= 16;
  }
  if (len > 0) {
    ctr = fio___gcm_inc_ctr(ctr);
    __m128i keystream = fio___aesni_encrypt256(ctr, rk);
    uint8_t ks_bytes[16];
    _mm_storeu_si128((__m128i *)ks_bytes, keystream);
    for (size_t i = 0; i < len; ++i)
      p[i] ^= ks_bytes[i];
    uint8_t tmp[16] = {0};
    FIO_MEMCPY(tmp, p, len);
    __m128i ct_block = _mm_loadu_si128((const __m128i *)tmp);
    ct_block = fio___bswap128(ct_block);
    tag = _mm_xor_si128(tag, ct_block);
    tag = fio___ghash_mult_pclmul(tag, h);
  }

  /* GHASH length block */
  uint8_t len_block[16] = {0};
  fio_u2buf64_be(len_block, (uint64_t)orig_adlen * 8);
  fio_u2buf64_be(len_block + 8, (uint64_t)orig_len * 8);
  __m128i len_blk = _mm_loadu_si128((const __m128i *)len_block);
  len_blk = fio___bswap128(len_blk);
  tag = _mm_xor_si128(tag, len_blk);
  tag = fio___ghash_mult_pclmul(tag, h);

  /* Final tag */
  __m128i s = fio___aesni_encrypt256(j0, rk);
  tag = fio___bswap128(tag);
  tag = _mm_xor_si128(tag, s);
  _mm_storeu_si128((__m128i *)mac, tag);
  /* Clear sensitive data */
  fio_secure_zero(rk, sizeof(rk));
  fio_secure_zero(htbl, sizeof(htbl));
  fio_secure_zero(j0_bytes, sizeof(j0_bytes));
}

SFUNC int fio_aes128_gcm_dec(void *restrict mac,
                             void *restrict data,
                             size_t len,
                             const void *ad,
                             size_t adlen,
                             const void *key,
                             const void *nonce) {
  __m128i rk[11];
  __m128i h, htbl[8], tag, ctr, j0;
  uint8_t *p = (uint8_t *)data;
  const uint8_t *aad = (const uint8_t *)ad;
  size_t orig_len = len;
  size_t orig_adlen = adlen;

  fio___aesni_gcm128_init(rk, &h, (const uint8_t *)key);
  h = fio___bswap128(h);

  /* Precompute H powers: H⁸ for 8-block, H⁴ for 4-block, H¹ for small */
  if (len >= 128 || adlen >= 128)
    fio___x86_ghash_precompute(h, htbl, 1);
  else if (len >= 64 || adlen >= 64)
    fio___x86_ghash_precompute(h, htbl, 0);
  else
    htbl[0] = h; /* Only H^1 needed for single-block path */

  uint8_t j0_bytes[16] = {0};
  FIO_MEMCPY(j0_bytes, nonce, 12);
  j0_bytes[15] = 1;
  j0 = _mm_loadu_si128((const __m128i *)j0_bytes);
  ctr = j0;
  tag = _mm_setzero_si128();

  /* GHASH over AAD - 8-block, 4-block, single */
  while (adlen >= 128) {
    __m128i a0 =
        _mm_xor_si128(tag,
                      fio___bswap128(_mm_loadu_si128((const __m128i *)aad)));
    __m128i a1 = fio___bswap128(_mm_loadu_si128((const __m128i *)(aad + 16)));
    __m128i a2 = fio___bswap128(_mm_loadu_si128((const __m128i *)(aad + 32)));
    __m128i a3 = fio___bswap128(_mm_loadu_si128((const __m128i *)(aad + 48)));
    __m128i a4 = fio___bswap128(_mm_loadu_si128((const __m128i *)(aad + 64)));
    __m128i a5 = fio___bswap128(_mm_loadu_si128((const __m128i *)(aad + 80)));
    __m128i a6 = fio___bswap128(_mm_loadu_si128((const __m128i *)(aad + 96)));
    __m128i a7 = fio___bswap128(_mm_loadu_si128((const __m128i *)(aad + 112)));
    tag = fio___x86_ghash_mult8(a0, a1, a2, a3, a4, a5, a6, a7, htbl);
    aad += 128;
    adlen -= 128;
  }
  while (adlen >= 64) {
    __m128i a0 = fio___bswap128(_mm_loadu_si128((const __m128i *)aad));
    __m128i a1 = fio___bswap128(_mm_loadu_si128((const __m128i *)(aad + 16)));
    __m128i a2 = fio___bswap128(_mm_loadu_si128((const __m128i *)(aad + 32)));
    __m128i a3 = fio___bswap128(_mm_loadu_si128((const __m128i *)(aad + 48)));
    a0 = _mm_xor_si128(tag, a0);
    tag = fio___x86_ghash_mult4(a0, a1, a2, a3, htbl);
    aad += 64;
    adlen -= 64;
  }
  while (adlen >= 16) {
    __m128i aad_block = _mm_loadu_si128((const __m128i *)aad);
    aad_block = fio___bswap128(aad_block);
    tag = _mm_xor_si128(tag, aad_block);
    tag = fio___ghash_mult_pclmul(tag, h);
    aad += 16;
    adlen -= 16;
  }
  if (adlen > 0) {
    uint8_t tmp[16] = {0};
    FIO_MEMCPY(tmp, aad, adlen);
    __m128i aad_block = _mm_loadu_si128((const __m128i *)tmp);
    aad_block = fio___bswap128(aad_block);
    tag = _mm_xor_si128(tag, aad_block);
    tag = fio___ghash_mult_pclmul(tag, h);
  }

  /* GHASH over ciphertext — 8-block, 4-block, single */
  const uint8_t *ct = p;
  size_t ct_len = orig_len;
  while (ct_len >= 128) {
    __m128i c0 =
        _mm_xor_si128(tag,
                      fio___bswap128(_mm_loadu_si128((const __m128i *)ct)));
    __m128i c1 = fio___bswap128(_mm_loadu_si128((const __m128i *)(ct + 16)));
    __m128i c2 = fio___bswap128(_mm_loadu_si128((const __m128i *)(ct + 32)));
    __m128i c3 = fio___bswap128(_mm_loadu_si128((const __m128i *)(ct + 48)));
    __m128i c4 = fio___bswap128(_mm_loadu_si128((const __m128i *)(ct + 64)));
    __m128i c5 = fio___bswap128(_mm_loadu_si128((const __m128i *)(ct + 80)));
    __m128i c6 = fio___bswap128(_mm_loadu_si128((const __m128i *)(ct + 96)));
    __m128i c7 = fio___bswap128(_mm_loadu_si128((const __m128i *)(ct + 112)));
    tag = fio___x86_ghash_mult8(c0, c1, c2, c3, c4, c5, c6, c7, htbl);
    ct += 128;
    ct_len -= 128;
  }
  while (ct_len >= 64) {
    __m128i c0 = fio___bswap128(_mm_loadu_si128((const __m128i *)ct));
    __m128i c1 = fio___bswap128(_mm_loadu_si128((const __m128i *)(ct + 16)));
    __m128i c2 = fio___bswap128(_mm_loadu_si128((const __m128i *)(ct + 32)));
    __m128i c3 = fio___bswap128(_mm_loadu_si128((const __m128i *)(ct + 48)));
    c0 = _mm_xor_si128(tag, c0);
    tag = fio___x86_ghash_mult4(c0, c1, c2, c3, htbl);
    ct += 64;
    ct_len -= 64;
  }
  while (ct_len >= 16) {
    __m128i ct_block = _mm_loadu_si128((const __m128i *)ct);
    ct_block = fio___bswap128(ct_block);
    tag = _mm_xor_si128(tag, ct_block);
    tag = fio___ghash_mult_pclmul(tag, h);
    ct += 16;
    ct_len -= 16;
  }
  if (ct_len > 0) {
    uint8_t tmp[16] = {0};
    FIO_MEMCPY(tmp, ct, ct_len);
    __m128i ct_block = _mm_loadu_si128((const __m128i *)tmp);
    ct_block = fio___bswap128(ct_block);
    tag = _mm_xor_si128(tag, ct_block);
    tag = fio___ghash_mult_pclmul(tag, h);
  }

  /* GHASH length block */
  uint8_t len_block[16] = {0};
  fio_u2buf64_be(len_block, (uint64_t)orig_adlen * 8);
  fio_u2buf64_be(len_block + 8, (uint64_t)orig_len * 8);
  __m128i len_blk = _mm_loadu_si128((const __m128i *)len_block);
  len_blk = fio___bswap128(len_blk);
  tag = _mm_xor_si128(tag, len_blk);
  tag = fio___ghash_mult_pclmul(tag, h);

  /* Compute and verify tag */
  __m128i s = fio___aesni_encrypt128(j0, rk);
  tag = fio___bswap128(tag);
  tag = _mm_xor_si128(tag, s);
  uint8_t computed_mac[16];
  _mm_storeu_si128((__m128i *)computed_mac, tag);
  if (!fio_ct_is_eq(computed_mac, mac, 16)) {
    fio_secure_zero(computed_mac, sizeof(computed_mac));
    fio_secure_zero(rk, sizeof(rk));
    fio_secure_zero(htbl, sizeof(htbl));
    fio_secure_zero(j0_bytes, sizeof(j0_bytes));
    return -1;
  }
  fio_secure_zero(computed_mac, sizeof(computed_mac));

  /* Decrypt — 8-block, 4-block, single-block, partial */
  while (len >= 128) {
    __m128i ctr0 = fio___gcm_inc_ctr(ctr);
    __m128i ctr1 = fio___gcm_inc_ctr(ctr0);
    __m128i ctr2 = fio___gcm_inc_ctr(ctr1);
    __m128i ctr3 = fio___gcm_inc_ctr(ctr2);
    __m128i ctr4 = fio___gcm_inc_ctr(ctr3);
    __m128i ctr5 = fio___gcm_inc_ctr(ctr4);
    __m128i ctr6 = fio___gcm_inc_ctr(ctr5);
    __m128i ctr7 = fio___gcm_inc_ctr(ctr6);
    ctr = ctr7;

    __m128i ks0 = fio___aesni_encrypt128(ctr0, rk);
    __m128i ks1 = fio___aesni_encrypt128(ctr1, rk);
    __m128i ks2 = fio___aesni_encrypt128(ctr2, rk);
    __m128i ks3 = fio___aesni_encrypt128(ctr3, rk);
    __m128i ks4 = fio___aesni_encrypt128(ctr4, rk);
    __m128i ks5 = fio___aesni_encrypt128(ctr5, rk);
    __m128i ks6 = fio___aesni_encrypt128(ctr6, rk);
    __m128i ks7 = fio___aesni_encrypt128(ctr7, rk);

    _mm_storeu_si128((__m128i *)p,
                     _mm_xor_si128(_mm_loadu_si128((const __m128i *)p), ks0));
    _mm_storeu_si128(
        (__m128i *)(p + 16),
        _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 16)), ks1));
    _mm_storeu_si128(
        (__m128i *)(p + 32),
        _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 32)), ks2));
    _mm_storeu_si128(
        (__m128i *)(p + 48),
        _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 48)), ks3));
    _mm_storeu_si128(
        (__m128i *)(p + 64),
        _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 64)), ks4));
    _mm_storeu_si128(
        (__m128i *)(p + 80),
        _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 80)), ks5));
    _mm_storeu_si128(
        (__m128i *)(p + 96),
        _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 96)), ks6));
    _mm_storeu_si128(
        (__m128i *)(p + 112),
        _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 112)), ks7));

    p += 128;
    len -= 128;
  }
  while (len >= 64) {
    __m128i ctr0 = fio___gcm_inc_ctr(ctr);
    __m128i ctr1 = fio___gcm_inc_ctr(ctr0);
    __m128i ctr2 = fio___gcm_inc_ctr(ctr1);
    __m128i ctr3 = fio___gcm_inc_ctr(ctr2);
    ctr = ctr3;

    __m128i ks0 = fio___aesni_encrypt128(ctr0, rk);
    __m128i ks1 = fio___aesni_encrypt128(ctr1, rk);
    __m128i ks2 = fio___aesni_encrypt128(ctr2, rk);
    __m128i ks3 = fio___aesni_encrypt128(ctr3, rk);

    _mm_storeu_si128((__m128i *)p,
                     _mm_xor_si128(_mm_loadu_si128((const __m128i *)p), ks0));
    _mm_storeu_si128(
        (__m128i *)(p + 16),
        _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 16)), ks1));
    _mm_storeu_si128(
        (__m128i *)(p + 32),
        _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 32)), ks2));
    _mm_storeu_si128(
        (__m128i *)(p + 48),
        _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 48)), ks3));

    p += 64;
    len -= 64;
  }
  while (len >= 16) {
    ctr = fio___gcm_inc_ctr(ctr);
    __m128i keystream = fio___aesni_encrypt128(ctr, rk);
    __m128i ciphertext = _mm_loadu_si128((const __m128i *)p);
    __m128i plaintext = _mm_xor_si128(ciphertext, keystream);
    _mm_storeu_si128((__m128i *)p, plaintext);
    p += 16;
    len -= 16;
  }
  if (len > 0) {
    ctr = fio___gcm_inc_ctr(ctr);
    __m128i keystream = fio___aesni_encrypt128(ctr, rk);
    uint8_t ks_bytes[16];
    _mm_storeu_si128((__m128i *)ks_bytes, keystream);
    for (size_t i = 0; i < len; ++i)
      p[i] ^= ks_bytes[i];
  }
  /* Clear sensitive data */
  fio_secure_zero(rk, sizeof(rk));
  fio_secure_zero(htbl, sizeof(htbl));
  fio_secure_zero(j0_bytes, sizeof(j0_bytes));
  return 0;
}

SFUNC int fio_aes256_gcm_dec(void *restrict mac,
                             void *restrict data,
                             size_t len,
                             const void *ad,
                             size_t adlen,
                             const void *key,
                             const void *nonce) {
  __m128i rk[15];
  __m128i h, htbl[8], tag, ctr, j0;
  uint8_t *p = (uint8_t *)data;
  const uint8_t *aad = (const uint8_t *)ad;
  size_t orig_len = len;
  size_t orig_adlen = adlen;

  fio___aesni_gcm256_init(rk, &h, (const uint8_t *)key);
  h = fio___bswap128(h);

  /* Precompute H powers: H⁸ for 8-block, H⁴ for 4-block, H¹ for small */
  if (len >= 128 || adlen >= 128)
    fio___x86_ghash_precompute(h, htbl, 1);
  else if (len >= 64 || adlen >= 64)
    fio___x86_ghash_precompute(h, htbl, 0);
  else
    htbl[0] = h; /* Only H^1 needed for single-block path */

  uint8_t j0_bytes[16] = {0};
  FIO_MEMCPY(j0_bytes, nonce, 12);
  j0_bytes[15] = 1;
  j0 = _mm_loadu_si128((const __m128i *)j0_bytes);
  ctr = j0;
  tag = _mm_setzero_si128();

  /* GHASH over AAD - 8-block, 4-block, single */
  while (adlen >= 128) {
    __m128i a0 =
        _mm_xor_si128(tag,
                      fio___bswap128(_mm_loadu_si128((const __m128i *)aad)));
    __m128i a1 = fio___bswap128(_mm_loadu_si128((const __m128i *)(aad + 16)));
    __m128i a2 = fio___bswap128(_mm_loadu_si128((const __m128i *)(aad + 32)));
    __m128i a3 = fio___bswap128(_mm_loadu_si128((const __m128i *)(aad + 48)));
    __m128i a4 = fio___bswap128(_mm_loadu_si128((const __m128i *)(aad + 64)));
    __m128i a5 = fio___bswap128(_mm_loadu_si128((const __m128i *)(aad + 80)));
    __m128i a6 = fio___bswap128(_mm_loadu_si128((const __m128i *)(aad + 96)));
    __m128i a7 = fio___bswap128(_mm_loadu_si128((const __m128i *)(aad + 112)));
    tag = fio___x86_ghash_mult8(a0, a1, a2, a3, a4, a5, a6, a7, htbl);
    aad += 128;
    adlen -= 128;
  }
  while (adlen >= 64) {
    __m128i a0 = fio___bswap128(_mm_loadu_si128((const __m128i *)aad));
    __m128i a1 = fio___bswap128(_mm_loadu_si128((const __m128i *)(aad + 16)));
    __m128i a2 = fio___bswap128(_mm_loadu_si128((const __m128i *)(aad + 32)));
    __m128i a3 = fio___bswap128(_mm_loadu_si128((const __m128i *)(aad + 48)));
    a0 = _mm_xor_si128(tag, a0);
    tag = fio___x86_ghash_mult4(a0, a1, a2, a3, htbl);
    aad += 64;
    adlen -= 64;
  }
  while (adlen >= 16) {
    __m128i aad_block = _mm_loadu_si128((const __m128i *)aad);
    aad_block = fio___bswap128(aad_block);
    tag = _mm_xor_si128(tag, aad_block);
    tag = fio___ghash_mult_pclmul(tag, h);
    aad += 16;
    adlen -= 16;
  }
  if (adlen > 0) {
    uint8_t tmp[16] = {0};
    FIO_MEMCPY(tmp, aad, adlen);
    __m128i aad_block = _mm_loadu_si128((const __m128i *)tmp);
    aad_block = fio___bswap128(aad_block);
    tag = _mm_xor_si128(tag, aad_block);
    tag = fio___ghash_mult_pclmul(tag, h);
  }

  /* GHASH over ciphertext — 8-block, 4-block, single */
  const uint8_t *ct = p;
  size_t ct_len = orig_len;
  while (ct_len >= 128) {
    __m128i c0 =
        _mm_xor_si128(tag,
                      fio___bswap128(_mm_loadu_si128((const __m128i *)ct)));
    __m128i c1 = fio___bswap128(_mm_loadu_si128((const __m128i *)(ct + 16)));
    __m128i c2 = fio___bswap128(_mm_loadu_si128((const __m128i *)(ct + 32)));
    __m128i c3 = fio___bswap128(_mm_loadu_si128((const __m128i *)(ct + 48)));
    __m128i c4 = fio___bswap128(_mm_loadu_si128((const __m128i *)(ct + 64)));
    __m128i c5 = fio___bswap128(_mm_loadu_si128((const __m128i *)(ct + 80)));
    __m128i c6 = fio___bswap128(_mm_loadu_si128((const __m128i *)(ct + 96)));
    __m128i c7 = fio___bswap128(_mm_loadu_si128((const __m128i *)(ct + 112)));
    tag = fio___x86_ghash_mult8(c0, c1, c2, c3, c4, c5, c6, c7, htbl);
    ct += 128;
    ct_len -= 128;
  }
  while (ct_len >= 64) {
    __m128i c0 = fio___bswap128(_mm_loadu_si128((const __m128i *)ct));
    __m128i c1 = fio___bswap128(_mm_loadu_si128((const __m128i *)(ct + 16)));
    __m128i c2 = fio___bswap128(_mm_loadu_si128((const __m128i *)(ct + 32)));
    __m128i c3 = fio___bswap128(_mm_loadu_si128((const __m128i *)(ct + 48)));
    c0 = _mm_xor_si128(tag, c0);
    tag = fio___x86_ghash_mult4(c0, c1, c2, c3, htbl);
    ct += 64;
    ct_len -= 64;
  }
  while (ct_len >= 16) {
    __m128i ct_block = _mm_loadu_si128((const __m128i *)ct);
    ct_block = fio___bswap128(ct_block);
    tag = _mm_xor_si128(tag, ct_block);
    tag = fio___ghash_mult_pclmul(tag, h);
    ct += 16;
    ct_len -= 16;
  }
  if (ct_len > 0) {
    uint8_t tmp[16] = {0};
    FIO_MEMCPY(tmp, ct, ct_len);
    __m128i ct_block = _mm_loadu_si128((const __m128i *)tmp);
    ct_block = fio___bswap128(ct_block);
    tag = _mm_xor_si128(tag, ct_block);
    tag = fio___ghash_mult_pclmul(tag, h);
  }

  /* GHASH length block */
  uint8_t len_block[16] = {0};
  fio_u2buf64_be(len_block, (uint64_t)orig_adlen * 8);
  fio_u2buf64_be(len_block + 8, (uint64_t)orig_len * 8);
  __m128i len_blk = _mm_loadu_si128((const __m128i *)len_block);
  len_blk = fio___bswap128(len_blk);
  tag = _mm_xor_si128(tag, len_blk);
  tag = fio___ghash_mult_pclmul(tag, h);

  /* Compute and verify tag */
  __m128i s = fio___aesni_encrypt256(j0, rk);
  tag = fio___bswap128(tag);
  tag = _mm_xor_si128(tag, s);
  uint8_t computed_mac[16];
  _mm_storeu_si128((__m128i *)computed_mac, tag);
  if (!fio_ct_is_eq(computed_mac, mac, 16)) {
    fio_secure_zero(computed_mac, sizeof(computed_mac));
    fio_secure_zero(rk, sizeof(rk));
    fio_secure_zero(htbl, sizeof(htbl));
    fio_secure_zero(j0_bytes, sizeof(j0_bytes));
    return -1;
  }
  fio_secure_zero(computed_mac, sizeof(computed_mac));

  /* Decrypt — 8-block, 4-block, single-block, partial */
  while (len >= 128) {
    __m128i ctr0 = fio___gcm_inc_ctr(ctr);
    __m128i ctr1 = fio___gcm_inc_ctr(ctr0);
    __m128i ctr2 = fio___gcm_inc_ctr(ctr1);
    __m128i ctr3 = fio___gcm_inc_ctr(ctr2);
    __m128i ctr4 = fio___gcm_inc_ctr(ctr3);
    __m128i ctr5 = fio___gcm_inc_ctr(ctr4);
    __m128i ctr6 = fio___gcm_inc_ctr(ctr5);
    __m128i ctr7 = fio___gcm_inc_ctr(ctr6);
    ctr = ctr7;

    __m128i ks0 = fio___aesni_encrypt256(ctr0, rk);
    __m128i ks1 = fio___aesni_encrypt256(ctr1, rk);
    __m128i ks2 = fio___aesni_encrypt256(ctr2, rk);
    __m128i ks3 = fio___aesni_encrypt256(ctr3, rk);
    __m128i ks4 = fio___aesni_encrypt256(ctr4, rk);
    __m128i ks5 = fio___aesni_encrypt256(ctr5, rk);
    __m128i ks6 = fio___aesni_encrypt256(ctr6, rk);
    __m128i ks7 = fio___aesni_encrypt256(ctr7, rk);

    _mm_storeu_si128((__m128i *)p,
                     _mm_xor_si128(_mm_loadu_si128((const __m128i *)p), ks0));
    _mm_storeu_si128(
        (__m128i *)(p + 16),
        _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 16)), ks1));
    _mm_storeu_si128(
        (__m128i *)(p + 32),
        _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 32)), ks2));
    _mm_storeu_si128(
        (__m128i *)(p + 48),
        _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 48)), ks3));
    _mm_storeu_si128(
        (__m128i *)(p + 64),
        _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 64)), ks4));
    _mm_storeu_si128(
        (__m128i *)(p + 80),
        _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 80)), ks5));
    _mm_storeu_si128(
        (__m128i *)(p + 96),
        _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 96)), ks6));
    _mm_storeu_si128(
        (__m128i *)(p + 112),
        _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 112)), ks7));

    p += 128;
    len -= 128;
  }
  while (len >= 64) {
    __m128i ctr0 = fio___gcm_inc_ctr(ctr);
    __m128i ctr1 = fio___gcm_inc_ctr(ctr0);
    __m128i ctr2 = fio___gcm_inc_ctr(ctr1);
    __m128i ctr3 = fio___gcm_inc_ctr(ctr2);
    ctr = ctr3;

    __m128i ks0 = fio___aesni_encrypt256(ctr0, rk);
    __m128i ks1 = fio___aesni_encrypt256(ctr1, rk);
    __m128i ks2 = fio___aesni_encrypt256(ctr2, rk);
    __m128i ks3 = fio___aesni_encrypt256(ctr3, rk);

    _mm_storeu_si128((__m128i *)p,
                     _mm_xor_si128(_mm_loadu_si128((const __m128i *)p), ks0));
    _mm_storeu_si128(
        (__m128i *)(p + 16),
        _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 16)), ks1));
    _mm_storeu_si128(
        (__m128i *)(p + 32),
        _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 32)), ks2));
    _mm_storeu_si128(
        (__m128i *)(p + 48),
        _mm_xor_si128(_mm_loadu_si128((const __m128i *)(p + 48)), ks3));

    p += 64;
    len -= 64;
  }
  while (len >= 16) {
    ctr = fio___gcm_inc_ctr(ctr);
    __m128i keystream = fio___aesni_encrypt256(ctr, rk);
    __m128i ciphertext = _mm_loadu_si128((const __m128i *)p);
    __m128i plaintext = _mm_xor_si128(ciphertext, keystream);
    _mm_storeu_si128((__m128i *)p, plaintext);
    p += 16;
    len -= 16;
  }
  if (len > 0) {
    ctr = fio___gcm_inc_ctr(ctr);
    __m128i keystream = fio___aesni_encrypt256(ctr, rk);
    uint8_t ks_bytes[16];
    _mm_storeu_si128((__m128i *)ks_bytes, keystream);
    for (size_t i = 0; i < len; ++i)
      p[i] ^= ks_bytes[i];
  }
  /* Clear sensitive data */
  fio_secure_zero(rk, sizeof(rk));
  fio_secure_zero(htbl, sizeof(htbl));
  fio_secure_zero(j0_bytes, sizeof(j0_bytes));
  return 0;
}

/* *****************************************************************************
ARM Crypto Extensions Implementation
***************************************************************************** */
#elif FIO___HAS_ARM_AES_INTRIN

/**
 * Constant-time SubWord using ARM AES hardware instructions.
 *
 * vaeseq_u8(data, zero) computes ShiftRows(SubBytes(data)).
 * By placing the 4 input bytes at positions {0, 5, 10, 15} (the diagonal),
 * ShiftRows maps them to column 0 (bytes {0, 1, 2, 3}).
 * This eliminates the S-box lookup table (cache-timing side channel).
 */
FIO_IFUNC uint32_t fio___arm_subword(uint32_t w) {
  /* Place bytes on the diagonal: positions {0, 5, 10, 15}.
   * Index 0xFF in vqtbl1q_u8 produces 0 (out-of-range → zero). */
  static const uint8_t diag_idx[16] = {0,
                                       0xFF,
                                       0xFF,
                                       0xFF,
                                       0xFF,
                                       1,
                                       0xFF,
                                       0xFF,
                                       0xFF,
                                       0xFF,
                                       2,
                                       0xFF,
                                       0xFF,
                                       0xFF,
                                       0xFF,
                                       3};
  /* Broadcast the 4-byte word into lane 0 of a 128-bit register */
  uint8x16_t src = vreinterpretq_u8_u32(vdupq_n_u32(w));
  /* Scatter to diagonal using TBL (out-of-range indices → 0) */
  uint8x16_t block = vqtbl1q_u8(src, vld1q_u8(diag_idx));
  /* vaeseq_u8 computes ShiftRows(SubBytes(block XOR zero)) */
  block = vaeseq_u8(block, vdupq_n_u8(0));
  /* Result: SubBytes output is now in bytes {0, 1, 2, 3} */
  return vgetq_lane_u32(vreinterpretq_u32_u8(block), 0);
}

FIO_IFUNC uint32_t fio___arm_rotword(uint32_t w) {
  return (w << 8) | (w >> 24);
}

/* ARM AES key expansion - stores keys as uint8x16_t */
FIO_IFUNC void fio___arm_aes128_key_expand(uint8x16_t *rk,
                                           const uint8_t key[16]) {
  static const uint32_t rcon[] = {0x01000000,
                                  0x02000000,
                                  0x04000000,
                                  0x08000000,
                                  0x10000000,
                                  0x20000000,
                                  0x40000000,
                                  0x80000000,
                                  0x1b000000,
                                  0x36000000};
  uint32_t w[44];

  for (int i = 0; i < 4; ++i)
    w[i] = fio_buf2u32_be(key + 4 * i);

  for (int i = 4; i < 44; ++i) {
    uint32_t tmp = w[i - 1];
    if ((i & 3) == 0)
      tmp = fio___arm_subword(fio___arm_rotword(tmp)) ^ rcon[(i / 4) - 1];
    w[i] = w[i - 4] ^ tmp;
  }

  /* Store as 11 round keys - convert to byte array for ARM */
  for (int i = 0; i < 11; ++i) {
    uint8_t tmp[16];
    fio_u2buf32_be(tmp + 0, w[i * 4 + 0]);
    fio_u2buf32_be(tmp + 4, w[i * 4 + 1]);
    fio_u2buf32_be(tmp + 8, w[i * 4 + 2]);
    fio_u2buf32_be(tmp + 12, w[i * 4 + 3]);
    rk[i] = vld1q_u8(tmp);
  }
}

FIO_IFUNC void fio___arm_aes256_key_expand(uint8x16_t *rk,
                                           const uint8_t key[32]) {
  static const uint32_t rcon[] = {0x01000000,
                                  0x02000000,
                                  0x04000000,
                                  0x08000000,
                                  0x10000000,
                                  0x20000000,
                                  0x40000000};
  uint32_t w[60];

  for (int i = 0; i < 8; ++i)
    w[i] = fio_buf2u32_be(key + 4 * i);

  for (int i = 8; i < 60; ++i) {
    uint32_t tmp = w[i - 1];
    if ((i & 7) == 0)
      tmp = fio___arm_subword(fio___arm_rotword(tmp)) ^ rcon[(i / 8) - 1];
    else if ((i & 7) == 4)
      tmp = fio___arm_subword(tmp);
    w[i] = w[i - 8] ^ tmp;
  }

  /* Store as 15 round keys - convert to byte array for ARM */
  for (int i = 0; i < 15; ++i) {
    uint8_t tmp[16];
    fio_u2buf32_be(tmp + 0, w[i * 4 + 0]);
    fio_u2buf32_be(tmp + 4, w[i * 4 + 1]);
    fio_u2buf32_be(tmp + 8, w[i * 4 + 2]);
    fio_u2buf32_be(tmp + 12, w[i * 4 + 3]);
    rk[i] = vld1q_u8(tmp);
  }
}

/* ARM AES block encryption
 * vaeseq_u8(data, key) = ShiftRows(SubBytes(data XOR key))
 * vaesmcq_u8(data) = MixColumns(data)
 *
 * Standard AES round: AddRoundKey -> SubBytes -> ShiftRows -> MixColumns
 * ARM sequence: XOR key, then AESE (which does SubBytes+ShiftRows), then AESMC
 */
FIO_IFUNC uint8x16_t fio___arm_aes128_encrypt(uint8x16_t block,
                                              const uint8x16_t *rk) {
  /* Rounds 1-9: AESE + AESMC */
  block = vaeseq_u8(block, rk[0]);
  block = vaesmcq_u8(block);
  block = vaeseq_u8(block, rk[1]);
  block = vaesmcq_u8(block);
  block = vaeseq_u8(block, rk[2]);
  block = vaesmcq_u8(block);
  block = vaeseq_u8(block, rk[3]);
  block = vaesmcq_u8(block);
  block = vaeseq_u8(block, rk[4]);
  block = vaesmcq_u8(block);
  block = vaeseq_u8(block, rk[5]);
  block = vaesmcq_u8(block);
  block = vaeseq_u8(block, rk[6]);
  block = vaesmcq_u8(block);
  block = vaeseq_u8(block, rk[7]);
  block = vaesmcq_u8(block);
  block = vaeseq_u8(block, rk[8]);
  block = vaesmcq_u8(block);
  /* Round 10: AESE (no MixColumns) + final XOR */
  block = vaeseq_u8(block, rk[9]);
  block = veorq_u8(block, rk[10]);
  return block;
}

FIO_IFUNC uint8x16_t fio___arm_aes256_encrypt(uint8x16_t block,
                                              const uint8x16_t *rk) {
  /* Rounds 1-13: AESE + AESMC */
  block = vaeseq_u8(block, rk[0]);
  block = vaesmcq_u8(block);
  block = vaeseq_u8(block, rk[1]);
  block = vaesmcq_u8(block);
  block = vaeseq_u8(block, rk[2]);
  block = vaesmcq_u8(block);
  block = vaeseq_u8(block, rk[3]);
  block = vaesmcq_u8(block);
  block = vaeseq_u8(block, rk[4]);
  block = vaesmcq_u8(block);
  block = vaeseq_u8(block, rk[5]);
  block = vaesmcq_u8(block);
  block = vaeseq_u8(block, rk[6]);
  block = vaesmcq_u8(block);
  block = vaeseq_u8(block, rk[7]);
  block = vaesmcq_u8(block);
  block = vaeseq_u8(block, rk[8]);
  block = vaesmcq_u8(block);
  block = vaeseq_u8(block, rk[9]);
  block = vaesmcq_u8(block);
  block = vaeseq_u8(block, rk[10]);
  block = vaesmcq_u8(block);
  block = vaeseq_u8(block, rk[11]);
  block = vaesmcq_u8(block);
  block = vaeseq_u8(block, rk[12]);
  block = vaesmcq_u8(block);
  /* Round 14: AESE (no MixColumns) + final XOR */
  block = vaeseq_u8(block, rk[13]);
  block = veorq_u8(block, rk[14]);
  return block;
}

/* GHASH multiplication in GF(2^128) using ARM PMULL
 *
 * All internal GHASH operations work in "bit-reversed" domain:
 * - H powers are stored pre-reversed (vrbitq_u8 applied once at precompute)
 * - The GHASH accumulator (tag) stays in bit-reversed form between iterations
 * - Input data blocks are reversed on entry (unavoidable)
 * - Final vrbitq_u8 converts back to GCM format only when extracting the tag
 *
 * This eliminates redundant vrbitq_u8 calls: instead of 3 per multiply
 * (2 inputs + 1 output), we only reverse input data blocks.
 *
 * GF(2^128) reduction is done entirely in NEON registers using 64-bit
 * shift + XOR instructions, avoiding expensive NEON↔scalar transfers.
 * Reduction polynomial: x^128 + x^7 + x^2 + x + 1.
 */

/**
 * NEON-only GF(2^128) reduction of a 256-bit product.
 * Input: lo (bits 0-127), hi (bits 128-255) of the product.
 * Output: 128-bit reduced result.
 * All operations use NEON 64-bit shift + XOR — no scalar extraction.
 */
FIO_IFUNC uint8x16_t fio___arm_ghash_reduce(uint64x2_t lo, uint64x2_t hi) {
  /* 256-bit product layout:
   * lo = {r0, r1} where r0 = bits 0-63, r1 = bits 64-127
   * hi = {r2, r3} where r2 = bits 128-191, r3 = bits 192-255
   *
   * Fold r3 into r2:r1 using x^128 ≡ x^7 + x^2 + x + 1 */

  /* Extract r3 as a vector for shifting */
  uint64x2_t r3 = vdupq_laneq_u64(hi, 1); /* {r3, r3} */

  /* r1 ^= r3 ^ (r3 << 7) ^ (r3 << 2) ^ (r3 << 1) */
  uint64x2_t r3_s7 = vshlq_n_u64(r3, 7);
  uint64x2_t r3_s2 = vshlq_n_u64(r3, 2);
  uint64x2_t r3_s1 = vshlq_n_u64(r3, 1);
  /* r2 ^= (r3 >> 57) ^ (r3 >> 62) ^ (r3 >> 63) */
  uint64x2_t r3_r57 = vshrq_n_u64(r3, 57);
  uint64x2_t r3_r62 = vshrq_n_u64(r3, 62);
  uint64x2_t r3_r63 = vshrq_n_u64(r3, 63);

  /* Apply r3 fold: update hi lane 0 (r2) and lo lane 1 (r1) */
  /* For r1: XOR r3, r3<<7, r3<<2, r3<<1 into lo lane 1 */
  /* For r2: XOR r3>>57, r3>>62, r3>>63 into hi lane 0 */
  uint64x2_t fold_lo = veorq_u64(r3, r3_s7);
  fold_lo = veorq_u64(fold_lo, r3_s2);
  fold_lo = veorq_u64(fold_lo, r3_s1);
  uint64x2_t fold_hi = veorq_u64(r3_r57, r3_r62);
  fold_hi = veorq_u64(fold_hi, r3_r63);

  /* lo lane 1 (r1) ^= fold_lo lane 0 */
  /* hi lane 0 (r2) ^= fold_hi lane 0 */
  /* Use vextq to create {fold_lo[0], 0} and XOR into lo at lane 1 position */
  uint64x2_t zero = vdupq_n_u64(0);
  uint64x2_t fold_r1 = vextq_u64(zero, fold_lo, 1); /* {0, fold_lo[0]} -> wrong
                                                     */
  /* Actually: vextq_u64(zero, fold_lo, 1) = {zero[1], fold_lo[0]} = {0,
   * fold_lo[0]} That puts fold_lo[0] in lane 1 — correct for r1! */
  lo = veorq_u64(lo, fold_r1);

  /* For hi: fold_hi[0] goes into hi lane 0 (r2).
   * Since r3 is broadcast, fold_hi = {val, val}. XOR into both lanes is fine
   * since we only use lane 0 after this. */
  hi = veorq_u64(hi, fold_hi);

  /* Now fold r2 (hi lane 0) into r1:r0 (lo)
   * r2 is in hi lane 0. */
  uint64x2_t r2 = vdupq_laneq_u64(hi, 0); /* {r2, r2} */

  uint64x2_t r2_s7 = vshlq_n_u64(r2, 7);
  uint64x2_t r2_s2 = vshlq_n_u64(r2, 2);
  uint64x2_t r2_s1 = vshlq_n_u64(r2, 1);
  uint64x2_t r2_r57 = vshrq_n_u64(r2, 57);
  uint64x2_t r2_r62 = vshrq_n_u64(r2, 62);
  uint64x2_t r2_r63 = vshrq_n_u64(r2, 63);

  /* r0 ^= r2 ^ (r2 << 7) ^ (r2 << 2) ^ (r2 << 1) — into lo lane 0 */
  uint64x2_t fold2_lo = veorq_u64(r2, r2_s7);
  fold2_lo = veorq_u64(fold2_lo, r2_s2);
  fold2_lo = veorq_u64(fold2_lo, r2_s1);
  /* r1 ^= (r2 >> 57) ^ (r2 >> 62) ^ (r2 >> 63) — into lo lane 1 */
  uint64x2_t fold2_hi = veorq_u64(r2_r57, r2_r62);
  fold2_hi = veorq_u64(fold2_hi, r2_r63);

  /* fold2_lo[0] -> lo lane 0, fold2_hi[0] -> lo lane 1 */
  /* Since r2 is broadcast, fold2_lo = {val_lo, val_lo}, fold2_hi = {val_hi,
   * val_hi} */
  /* We need: lo[0] ^= val_lo, lo[1] ^= val_hi */
  /* Construct {val_lo, val_hi} using vzip/vcombine */
  uint64x2_t fold2_combined =
      vcombine_u64(vget_low_u64(fold2_lo), vget_low_u64(fold2_hi));
  lo = veorq_u64(lo, fold2_combined);

  return vreinterpretq_u8_u64(lo);
}

/**
 * Single-block GHASH multiply in bit-reversed domain.
 * Both inputs (a, b) must already be in bit-reversed form.
 * Result is in bit-reversed form (no vrbitq_u8 on input or output).
 * Uses Karatsuba decomposition (3 PMULL) + NEON-only reduction.
 */
FIO_IFUNC uint8x16_t fio___arm_ghash_mult_br(uint8x16_t a_br, uint8x16_t b_br) {
  /* In bit-reversed form:
   * lane 0 = x^0 to x^63 (low), lane 1 = x^64 to x^127 (high) */
  poly64_t a_lo = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(a_br), 0);
  poly64_t a_hi = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(a_br), 1);
  poly64_t b_lo = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(b_br), 0);
  poly64_t b_hi = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(b_br), 1);

  /* Karatsuba: (a_hi*x^64 + a_lo) * (b_hi*x^64 + b_lo) */
  poly128_t p_ll = vmull_p64(a_lo, b_lo);
  poly128_t p_hh = vmull_p64(a_hi, b_hi);
  poly128_t p_mid = vmull_p64((poly64_t)(a_lo ^ a_hi), (poly64_t)(b_lo ^ b_hi));

  uint64x2_t ll = vreinterpretq_u64_p128(p_ll);
  uint64x2_t hh = vreinterpretq_u64_p128(p_hh);
  uint64x2_t mm = vreinterpretq_u64_p128(p_mid);
  mm = veorq_u64(mm, ll);
  mm = veorq_u64(mm, hh);

  /* Assemble 256-bit product into lo (bits 0-127) and hi (bits 128-255) */
  /* lo = {ll[0], ll[1] ^ mm[0]}, hi = {hh[0] ^ mm[1], hh[1]} */
  uint64x2_t lo = veorq_u64(ll, vextq_u64(vdupq_n_u64(0), mm, 1));
  uint64x2_t hi = veorq_u64(hh, vextq_u64(mm, vdupq_n_u64(0), 1));

  return fio___arm_ghash_reduce(lo, hi);
}

/**
 * GHASH multiply with GCM-format inputs (applies vrbitq_u8 to both inputs).
 * Used for single-block GHASH where tag is in GCM format.
 * Result is in GCM format.
 */
FIO_IFUNC uint8x16_t fio___arm_ghash_mult(uint8x16_t x_vec, uint8x16_t h_vec) {
  uint8x16_t result =
      fio___arm_ghash_mult_br(vrbitq_u8(x_vec), vrbitq_u8(h_vec));
  return vrbitq_u8(result);
}

/**
 * Precompute H powers for parallel GHASH.
 * All stored in bit-reversed form for use with fio___arm_ghash_mult_br.
 * htbl[0] = H^1_br, htbl[1] = H^2_br, ..., htbl[n-1] = H^n_br.
 *
 * When compute8 is true, computes H¹ through H⁸ (for 8-block GHASH).
 * When false, computes H¹ through H⁴ (for 4-block GHASH).
 */
FIO_IFUNC void fio___arm_ghash_precompute(uint8x16_t h,
                                          uint8x16_t *htbl,
                                          int compute8) {
  uint8x16_t h_br = vrbitq_u8(h);
  htbl[0] = h_br;                                      /* H¹_br */
  htbl[1] = fio___arm_ghash_mult_br(h_br, h_br);       /* H²_br */
  htbl[2] = fio___arm_ghash_mult_br(htbl[1], h_br);    /* H³_br */
  htbl[3] = fio___arm_ghash_mult_br(htbl[1], htbl[1]); /* H⁴_br */
  if (compute8) {
    htbl[4] = fio___arm_ghash_mult_br(htbl[3], h_br);    /* H⁵_br */
    htbl[5] = fio___arm_ghash_mult_br(htbl[3], htbl[1]); /* H⁶_br */
    htbl[6] = fio___arm_ghash_mult_br(htbl[3], htbl[2]); /* H⁷_br */
    htbl[7] = fio___arm_ghash_mult_br(htbl[3], htbl[3]); /* H⁸_br */
  }
}

/**
 * 4-way parallel GHASH with deferred reduction.
 * Inputs x0..x3 are in bit-reversed form, htbl contains bit-reversed H powers.
 * Accumulates all 4 Karatsuba partial products, then does a single reduction.
 * Saves 3 reductions per 4-block batch vs calling fio___arm_ghash_mult_br 4x.
 *
 * htbl[3] = H^4_br (for x0), htbl[2] = H^3_br, htbl[1] = H^2_br,
 * htbl[0] = H^1_br (for x3).
 */
FIO_IFUNC uint8x16_t fio___arm_ghash_mult4(uint8x16_t x0,
                                           uint8x16_t x1,
                                           uint8x16_t x2,
                                           uint8x16_t x3,
                                           const uint8x16_t *htbl) {
  uint64x2_t acc_lo, acc_hi, acc_mm;

  /* Block 0: x0 * H^4 */
  {
    poly64_t a_lo = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(x0), 0);
    poly64_t a_hi = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(x0), 1);
    poly64_t b_lo = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(htbl[3]), 0);
    poly64_t b_hi = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(htbl[3]), 1);
    acc_lo = vreinterpretq_u64_p128(vmull_p64(a_lo, b_lo));
    acc_hi = vreinterpretq_u64_p128(vmull_p64(a_hi, b_hi));
    acc_mm = vreinterpretq_u64_p128(
        vmull_p64((poly64_t)(a_lo ^ a_hi), (poly64_t)(b_lo ^ b_hi)));
  }

  /* Block 1: x1 * H^3 — accumulate via XOR */
  {
    poly64_t a_lo = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(x1), 0);
    poly64_t a_hi = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(x1), 1);
    poly64_t b_lo = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(htbl[2]), 0);
    poly64_t b_hi = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(htbl[2]), 1);
    acc_lo = veorq_u64(acc_lo, vreinterpretq_u64_p128(vmull_p64(a_lo, b_lo)));
    acc_hi = veorq_u64(acc_hi, vreinterpretq_u64_p128(vmull_p64(a_hi, b_hi)));
    acc_mm =
        veorq_u64(acc_mm,
                  vreinterpretq_u64_p128(vmull_p64((poly64_t)(a_lo ^ a_hi),
                                                   (poly64_t)(b_lo ^ b_hi))));
  }

  /* Block 2: x2 * H^2 */
  {
    poly64_t a_lo = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(x2), 0);
    poly64_t a_hi = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(x2), 1);
    poly64_t b_lo = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(htbl[1]), 0);
    poly64_t b_hi = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(htbl[1]), 1);
    acc_lo = veorq_u64(acc_lo, vreinterpretq_u64_p128(vmull_p64(a_lo, b_lo)));
    acc_hi = veorq_u64(acc_hi, vreinterpretq_u64_p128(vmull_p64(a_hi, b_hi)));
    acc_mm =
        veorq_u64(acc_mm,
                  vreinterpretq_u64_p128(vmull_p64((poly64_t)(a_lo ^ a_hi),
                                                   (poly64_t)(b_lo ^ b_hi))));
  }

  /* Block 3: x3 * H^1 */
  {
    poly64_t a_lo = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(x3), 0);
    poly64_t a_hi = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(x3), 1);
    poly64_t b_lo = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(htbl[0]), 0);
    poly64_t b_hi = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(htbl[0]), 1);
    acc_lo = veorq_u64(acc_lo, vreinterpretq_u64_p128(vmull_p64(a_lo, b_lo)));
    acc_hi = veorq_u64(acc_hi, vreinterpretq_u64_p128(vmull_p64(a_hi, b_hi)));
    acc_mm =
        veorq_u64(acc_mm,
                  vreinterpretq_u64_p128(vmull_p64((poly64_t)(a_lo ^ a_hi),
                                                   (poly64_t)(b_lo ^ b_hi))));
  }

  /* Karatsuba combination on accumulated sums */
  acc_mm = veorq_u64(acc_mm, acc_lo);
  acc_mm = veorq_u64(acc_mm, acc_hi);

  /* Assemble 256-bit product: lo = {acc_lo[0], acc_lo[1]^acc_mm[0]},
   *                           hi = {acc_hi[0]^acc_mm[1], acc_hi[1]} */
  uint64x2_t zero = vdupq_n_u64(0);
  uint64x2_t lo = veorq_u64(acc_lo, vextq_u64(zero, acc_mm, 1));
  uint64x2_t hi = veorq_u64(acc_hi, vextq_u64(acc_mm, zero, 1));

  /* Single reduction for all 4 blocks */
  return fio___arm_ghash_reduce(lo, hi);
}

/**
 * 8-way parallel GHASH with deferred reduction.
 * Inputs x0..x7 are in bit-reversed form, htbl contains bit-reversed H powers.
 * Accumulates all 8 Karatsuba partial products, then does a single reduction.
 * Saves 7 reductions per 8-block batch vs calling fio___arm_ghash_mult_br 8x.
 *
 * htbl[7] = H^8_br (for x0), htbl[6] = H^7_br, ..., htbl[0] = H^1_br (for
 * x7).
 */
FIO_IFUNC uint8x16_t fio___arm_ghash_mult8(uint8x16_t x0,
                                           uint8x16_t x1,
                                           uint8x16_t x2,
                                           uint8x16_t x3,
                                           uint8x16_t x4,
                                           uint8x16_t x5,
                                           uint8x16_t x6,
                                           uint8x16_t x7,
                                           const uint8x16_t *htbl) {
  uint64x2_t acc_lo, acc_hi, acc_mm;

  /* Block 0: x0 * H^8 */
  {
    poly64_t a_lo = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(x0), 0);
    poly64_t a_hi = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(x0), 1);
    poly64_t b_lo = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(htbl[7]), 0);
    poly64_t b_hi = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(htbl[7]), 1);
    acc_lo = vreinterpretq_u64_p128(vmull_p64(a_lo, b_lo));
    acc_hi = vreinterpretq_u64_p128(vmull_p64(a_hi, b_hi));
    acc_mm = vreinterpretq_u64_p128(
        vmull_p64((poly64_t)(a_lo ^ a_hi), (poly64_t)(b_lo ^ b_hi)));
  }

  /* Block 1: x1 * H^7 */
  {
    poly64_t a_lo = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(x1), 0);
    poly64_t a_hi = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(x1), 1);
    poly64_t b_lo = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(htbl[6]), 0);
    poly64_t b_hi = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(htbl[6]), 1);
    acc_lo = veorq_u64(acc_lo, vreinterpretq_u64_p128(vmull_p64(a_lo, b_lo)));
    acc_hi = veorq_u64(acc_hi, vreinterpretq_u64_p128(vmull_p64(a_hi, b_hi)));
    acc_mm =
        veorq_u64(acc_mm,
                  vreinterpretq_u64_p128(vmull_p64((poly64_t)(a_lo ^ a_hi),
                                                   (poly64_t)(b_lo ^ b_hi))));
  }

  /* Block 2: x2 * H^6 */
  {
    poly64_t a_lo = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(x2), 0);
    poly64_t a_hi = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(x2), 1);
    poly64_t b_lo = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(htbl[5]), 0);
    poly64_t b_hi = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(htbl[5]), 1);
    acc_lo = veorq_u64(acc_lo, vreinterpretq_u64_p128(vmull_p64(a_lo, b_lo)));
    acc_hi = veorq_u64(acc_hi, vreinterpretq_u64_p128(vmull_p64(a_hi, b_hi)));
    acc_mm =
        veorq_u64(acc_mm,
                  vreinterpretq_u64_p128(vmull_p64((poly64_t)(a_lo ^ a_hi),
                                                   (poly64_t)(b_lo ^ b_hi))));
  }

  /* Block 3: x3 * H^5 */
  {
    poly64_t a_lo = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(x3), 0);
    poly64_t a_hi = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(x3), 1);
    poly64_t b_lo = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(htbl[4]), 0);
    poly64_t b_hi = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(htbl[4]), 1);
    acc_lo = veorq_u64(acc_lo, vreinterpretq_u64_p128(vmull_p64(a_lo, b_lo)));
    acc_hi = veorq_u64(acc_hi, vreinterpretq_u64_p128(vmull_p64(a_hi, b_hi)));
    acc_mm =
        veorq_u64(acc_mm,
                  vreinterpretq_u64_p128(vmull_p64((poly64_t)(a_lo ^ a_hi),
                                                   (poly64_t)(b_lo ^ b_hi))));
  }

  /* Block 4: x4 * H^4 */
  {
    poly64_t a_lo = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(x4), 0);
    poly64_t a_hi = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(x4), 1);
    poly64_t b_lo = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(htbl[3]), 0);
    poly64_t b_hi = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(htbl[3]), 1);
    acc_lo = veorq_u64(acc_lo, vreinterpretq_u64_p128(vmull_p64(a_lo, b_lo)));
    acc_hi = veorq_u64(acc_hi, vreinterpretq_u64_p128(vmull_p64(a_hi, b_hi)));
    acc_mm =
        veorq_u64(acc_mm,
                  vreinterpretq_u64_p128(vmull_p64((poly64_t)(a_lo ^ a_hi),
                                                   (poly64_t)(b_lo ^ b_hi))));
  }

  /* Block 5: x5 * H^3 */
  {
    poly64_t a_lo = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(x5), 0);
    poly64_t a_hi = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(x5), 1);
    poly64_t b_lo = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(htbl[2]), 0);
    poly64_t b_hi = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(htbl[2]), 1);
    acc_lo = veorq_u64(acc_lo, vreinterpretq_u64_p128(vmull_p64(a_lo, b_lo)));
    acc_hi = veorq_u64(acc_hi, vreinterpretq_u64_p128(vmull_p64(a_hi, b_hi)));
    acc_mm =
        veorq_u64(acc_mm,
                  vreinterpretq_u64_p128(vmull_p64((poly64_t)(a_lo ^ a_hi),
                                                   (poly64_t)(b_lo ^ b_hi))));
  }

  /* Block 6: x6 * H^2 */
  {
    poly64_t a_lo = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(x6), 0);
    poly64_t a_hi = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(x6), 1);
    poly64_t b_lo = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(htbl[1]), 0);
    poly64_t b_hi = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(htbl[1]), 1);
    acc_lo = veorq_u64(acc_lo, vreinterpretq_u64_p128(vmull_p64(a_lo, b_lo)));
    acc_hi = veorq_u64(acc_hi, vreinterpretq_u64_p128(vmull_p64(a_hi, b_hi)));
    acc_mm =
        veorq_u64(acc_mm,
                  vreinterpretq_u64_p128(vmull_p64((poly64_t)(a_lo ^ a_hi),
                                                   (poly64_t)(b_lo ^ b_hi))));
  }

  /* Block 7: x7 * H^1 */
  {
    poly64_t a_lo = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(x7), 0);
    poly64_t a_hi = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(x7), 1);
    poly64_t b_lo = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(htbl[0]), 0);
    poly64_t b_hi = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(htbl[0]), 1);
    acc_lo = veorq_u64(acc_lo, vreinterpretq_u64_p128(vmull_p64(a_lo, b_lo)));
    acc_hi = veorq_u64(acc_hi, vreinterpretq_u64_p128(vmull_p64(a_hi, b_hi)));
    acc_mm =
        veorq_u64(acc_mm,
                  vreinterpretq_u64_p128(vmull_p64((poly64_t)(a_lo ^ a_hi),
                                                   (poly64_t)(b_lo ^ b_hi))));
  }

  /* Karatsuba combination on accumulated sums */
  acc_mm = veorq_u64(acc_mm, acc_lo);
  acc_mm = veorq_u64(acc_mm, acc_hi);

  /* Assemble 256-bit product */
  uint64x2_t zero = vdupq_n_u64(0);
  uint64x2_t lo = veorq_u64(acc_lo, vextq_u64(zero, acc_mm, 1));
  uint64x2_t hi = veorq_u64(acc_hi, vextq_u64(acc_mm, zero, 1));

  /* Single reduction for all 8 blocks */
  return fio___arm_ghash_reduce(lo, hi);
}

/**
 * Single-block GHASH multiply for the GCM API.
 * tag_br is in bit-reversed form, h_br is bit-reversed H^1.
 * Returns result in bit-reversed form.
 */
FIO_IFUNC uint8x16_t fio___arm_ghash_mult1_br(uint8x16_t tag_br,
                                              const uint8x16_t *htbl) {
  return fio___arm_ghash_mult_br(tag_br, htbl[0]);
}

/* === Interleaved AES+GHASH macros for 8-block pipeline ===
 *
 * These macros perform AES rounds on 8 blocks while interleaving GHASH
 * Karatsuba partial product accumulations. The CPU's out-of-order engine
 * overlaps the independent AES and PMULL instruction chains.
 *
 * AES round on 8 blocks (except last round which skips MixColumns):
 */
#define FIO___ARM_AES_ROUND8(c0, c1, c2, c3, c4, c5, c6, c7, rk_i)             \
  do {                                                                         \
    c0 = vaesmcq_u8(vaeseq_u8(c0, rk_i));                                      \
    c1 = vaesmcq_u8(vaeseq_u8(c1, rk_i));                                      \
    c2 = vaesmcq_u8(vaeseq_u8(c2, rk_i));                                      \
    c3 = vaesmcq_u8(vaeseq_u8(c3, rk_i));                                      \
    c4 = vaesmcq_u8(vaeseq_u8(c4, rk_i));                                      \
    c5 = vaesmcq_u8(vaeseq_u8(c5, rk_i));                                      \
    c6 = vaesmcq_u8(vaeseq_u8(c6, rk_i));                                      \
    c7 = vaesmcq_u8(vaeseq_u8(c7, rk_i));                                      \
  } while (0)

/* Final AES round (AESE + XOR, no MixColumns) */
#define FIO___ARM_AES_LAST8(c0, c1, c2, c3, c4, c5, c6, c7, rk_2nd, rk_last)   \
  do {                                                                         \
    c0 = veorq_u8(vaeseq_u8(c0, rk_2nd), rk_last);                             \
    c1 = veorq_u8(vaeseq_u8(c1, rk_2nd), rk_last);                             \
    c2 = veorq_u8(vaeseq_u8(c2, rk_2nd), rk_last);                             \
    c3 = veorq_u8(vaeseq_u8(c3, rk_2nd), rk_last);                             \
    c4 = veorq_u8(vaeseq_u8(c4, rk_2nd), rk_last);                             \
    c5 = veorq_u8(vaeseq_u8(c5, rk_2nd), rk_last);                             \
    c6 = veorq_u8(vaeseq_u8(c6, rk_2nd), rk_last);                             \
    c7 = veorq_u8(vaeseq_u8(c7, rk_2nd), rk_last);                             \
  } while (0)

/* GHASH Karatsuba partial product for one block — accumulate into accumulators.
 * This is the inner operation of fio___arm_ghash_mult8, broken out so it can
 * be interleaved with AES rounds. */
#define FIO___ARM_GHASH_ACCUM(acc_lo, acc_hi, acc_mm, x_br, h_br)              \
  do {                                                                         \
    poly64_t ga_lo_ = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(x_br), 0); \
    poly64_t ga_hi_ = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(x_br), 1); \
    poly64_t gb_lo_ = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(h_br), 0); \
    poly64_t gb_hi_ = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(h_br), 1); \
    acc_lo =                                                                   \
        veorq_u64(acc_lo, vreinterpretq_u64_p128(vmull_p64(ga_lo_, gb_lo_)));  \
    acc_hi =                                                                   \
        veorq_u64(acc_hi, vreinterpretq_u64_p128(vmull_p64(ga_hi_, gb_hi_)));  \
    acc_mm = veorq_u64(                                                        \
        acc_mm,                                                                \
        vreinterpretq_u64_p128(vmull_p64((poly64_t)(ga_lo_ ^ ga_hi_),          \
                                         (poly64_t)(gb_lo_ ^ gb_hi_))));       \
  } while (0)

/* GHASH Karatsuba FIRST block — initialize accumulators */
#define FIO___ARM_GHASH_INIT(acc_lo, acc_hi, acc_mm, x_br, h_br)               \
  do {                                                                         \
    poly64_t gi_lo_ = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(x_br), 0); \
    poly64_t gi_hi_ = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(x_br), 1); \
    poly64_t gj_lo_ = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(h_br), 0); \
    poly64_t gj_hi_ = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(h_br), 1); \
    acc_lo = vreinterpretq_u64_p128(vmull_p64(gi_lo_, gj_lo_));                \
    acc_hi = vreinterpretq_u64_p128(vmull_p64(gi_hi_, gj_hi_));                \
    acc_mm = vreinterpretq_u64_p128(                                           \
        vmull_p64((poly64_t)(gi_lo_ ^ gi_hi_), (poly64_t)(gj_lo_ ^ gj_hi_)));  \
  } while (0)

/* Finalize GHASH: Karatsuba combination + reduction */
#define FIO___ARM_GHASH_FINAL(result, acc_lo, acc_hi, acc_mm)                  \
  do {                                                                         \
    acc_mm = veorq_u64(acc_mm, acc_lo);                                        \
    acc_mm = veorq_u64(acc_mm, acc_hi);                                        \
    uint64x2_t gz_ = vdupq_n_u64(0);                                           \
    uint64x2_t glo_ = veorq_u64(acc_lo, vextq_u64(gz_, acc_mm, 1));            \
    uint64x2_t ghi_ = veorq_u64(acc_hi, vextq_u64(acc_mm, gz_, 1));            \
    result = fio___arm_ghash_reduce(glo_, ghi_);                               \
  } while (0)

/* Byte reverse for GCM (convert between big-endian and native) */
FIO_IFUNC uint8x16_t fio___arm_bswap128(uint8x16_t x) {
  static const uint8_t rev_idx[16] =
      {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
  return vqtbl1q_u8(x, vld1q_u8(rev_idx));
}

/* Increment counter (last 32 bits, big-endian) using NEON */
FIO_IFUNC uint8x16_t fio___arm_gcm_inc_ctr(uint8x16_t ctr) {
  /* The counter is in bytes 12-15, big-endian.
   * We need to: extract, byte-swap, increment, byte-swap, insert */
  uint32x4_t ctr32 = vreinterpretq_u32_u8(ctr);
  uint32_t c = vgetq_lane_u32(ctr32, 3);
  c = fio_bswap32(fio_bswap32(c) + 1);
  return vreinterpretq_u8_u32(vsetq_lane_u32(c, ctr32, 3));
}

SFUNC void fio_aes128_gcm_enc(void *restrict mac,
                              void *restrict data,
                              size_t len,
                              const void *ad,
                              size_t adlen,
                              const void *key,
                              const void *nonce) {
  uint8x16_t rk[11];
  uint8x16_t htbl[8], tag_br, ctr, j0;
  uint8_t *p = (uint8_t *)data;
  const uint8_t *aad = (const uint8_t *)ad;
  size_t orig_len = len;
  size_t orig_adlen = adlen;

  fio___arm_aes128_key_expand(rk, (const uint8_t *)key);
  uint8x16_t zero = vdupq_n_u8(0);
  uint8x16_t h = fio___arm_aes128_encrypt(zero, rk);

  /* Precompute H powers: H⁸ for 8-block, H⁴ for 4-block, H¹ for small */
  if (len >= 128 || adlen >= 128)
    fio___arm_ghash_precompute(h, htbl, 1);
  else if (len >= 64 || adlen >= 64)
    fio___arm_ghash_precompute(h, htbl, 0);
  else
    htbl[0] = vrbitq_u8(h); /* Only H^1_br needed for single-block path */

  uint8_t j0_bytes[16] = {0};
  FIO_MEMCPY(j0_bytes, nonce, 12);
  j0_bytes[15] = 1;
  j0 = vld1q_u8(j0_bytes);
  ctr = j0;
  tag_br =
      vdupq_n_u8(0); /* tag in bit-reversed domain (zero is self-inverse) */

  /* GHASH over AAD - process 8 blocks, then 4 blocks, then single */
  while (adlen >= 128) {
    uint8x16_t a0 = veorq_u8(tag_br, vrbitq_u8(vld1q_u8(aad)));
    uint8x16_t a1 = vrbitq_u8(vld1q_u8(aad + 16));
    uint8x16_t a2 = vrbitq_u8(vld1q_u8(aad + 32));
    uint8x16_t a3 = vrbitq_u8(vld1q_u8(aad + 48));
    uint8x16_t a4 = vrbitq_u8(vld1q_u8(aad + 64));
    uint8x16_t a5 = vrbitq_u8(vld1q_u8(aad + 80));
    uint8x16_t a6 = vrbitq_u8(vld1q_u8(aad + 96));
    uint8x16_t a7 = vrbitq_u8(vld1q_u8(aad + 112));
    tag_br = fio___arm_ghash_mult8(a0, a1, a2, a3, a4, a5, a6, a7, htbl);
    aad += 128;
    adlen -= 128;
  }
  while (adlen >= 64) {
    uint8x16_t a0 = veorq_u8(tag_br, vrbitq_u8(vld1q_u8(aad)));
    uint8x16_t a1 = vrbitq_u8(vld1q_u8(aad + 16));
    uint8x16_t a2 = vrbitq_u8(vld1q_u8(aad + 32));
    uint8x16_t a3 = vrbitq_u8(vld1q_u8(aad + 48));
    tag_br = fio___arm_ghash_mult4(a0, a1, a2, a3, htbl);
    aad += 64;
    adlen -= 64;
  }
  while (adlen >= 16) {
    tag_br = veorq_u8(tag_br, vrbitq_u8(vld1q_u8(aad)));
    tag_br = fio___arm_ghash_mult1_br(tag_br, htbl);
    aad += 16;
    adlen -= 16;
  }
  if (adlen > 0) {
    uint8_t tmp[16] = {0};
    FIO_MEMCPY(tmp, aad, adlen);
    tag_br = veorq_u8(tag_br, vrbitq_u8(vld1q_u8(tmp)));
    tag_br = fio___arm_ghash_mult1_br(tag_br, htbl);
  }

  /* === 8-block interleaved AES-CTR encryption + GHASH === */
  if (len >= 256) {
    /* Prologue: encrypt first 8 blocks (no previous ciphertext to GHASH) */
    uint8x16_t ctr0 = fio___arm_gcm_inc_ctr(ctr);
    uint8x16_t ctr1 = fio___arm_gcm_inc_ctr(ctr0);
    uint8x16_t ctr2 = fio___arm_gcm_inc_ctr(ctr1);
    uint8x16_t ctr3 = fio___arm_gcm_inc_ctr(ctr2);
    uint8x16_t ctr4 = fio___arm_gcm_inc_ctr(ctr3);
    uint8x16_t ctr5 = fio___arm_gcm_inc_ctr(ctr4);
    uint8x16_t ctr6 = fio___arm_gcm_inc_ctr(ctr5);
    uint8x16_t ctr7 = fio___arm_gcm_inc_ctr(ctr6);
    ctr = ctr7;

    uint8x16_t ct0 = veorq_u8(vld1q_u8(p), fio___arm_aes128_encrypt(ctr0, rk));
    uint8x16_t ct1 =
        veorq_u8(vld1q_u8(p + 16), fio___arm_aes128_encrypt(ctr1, rk));
    uint8x16_t ct2 =
        veorq_u8(vld1q_u8(p + 32), fio___arm_aes128_encrypt(ctr2, rk));
    uint8x16_t ct3 =
        veorq_u8(vld1q_u8(p + 48), fio___arm_aes128_encrypt(ctr3, rk));
    uint8x16_t ct4 =
        veorq_u8(vld1q_u8(p + 64), fio___arm_aes128_encrypt(ctr4, rk));
    uint8x16_t ct5 =
        veorq_u8(vld1q_u8(p + 80), fio___arm_aes128_encrypt(ctr5, rk));
    uint8x16_t ct6 =
        veorq_u8(vld1q_u8(p + 96), fio___arm_aes128_encrypt(ctr6, rk));
    uint8x16_t ct7 =
        veorq_u8(vld1q_u8(p + 112), fio___arm_aes128_encrypt(ctr7, rk));

    vst1q_u8(p, ct0);
    vst1q_u8(p + 16, ct1);
    vst1q_u8(p + 32, ct2);
    vst1q_u8(p + 48, ct3);
    vst1q_u8(p + 64, ct4);
    vst1q_u8(p + 80, ct5);
    vst1q_u8(p + 96, ct6);
    vst1q_u8(p + 112, ct7);

    /* Save previous ciphertext (bit-reversed) for GHASH in next iteration */
    uint8x16_t prev0_br = veorq_u8(tag_br, vrbitq_u8(ct0));
    uint8x16_t prev1_br = vrbitq_u8(ct1);
    uint8x16_t prev2_br = vrbitq_u8(ct2);
    uint8x16_t prev3_br = vrbitq_u8(ct3);
    uint8x16_t prev4_br = vrbitq_u8(ct4);
    uint8x16_t prev5_br = vrbitq_u8(ct5);
    uint8x16_t prev6_br = vrbitq_u8(ct6);
    uint8x16_t prev7_br = vrbitq_u8(ct7);

    p += 128;
    len -= 128;

    /* Steady state: interleaved AES-128 encrypt + GHASH.
     * AES rounds and GHASH Karatsuba products are interleaved at the macro
     * level so the CPU's OoO engine overlaps the independent chains. */
    while (len >= 128) {
      uint64x2_t gh_lo, gh_hi, gh_mm;

      /* Prepare 8 new counter blocks */
      ctr0 = fio___arm_gcm_inc_ctr(ctr);
      ctr1 = fio___arm_gcm_inc_ctr(ctr0);
      ctr2 = fio___arm_gcm_inc_ctr(ctr1);
      ctr3 = fio___arm_gcm_inc_ctr(ctr2);
      ctr4 = fio___arm_gcm_inc_ctr(ctr3);
      ctr5 = fio___arm_gcm_inc_ctr(ctr4);
      ctr6 = fio___arm_gcm_inc_ctr(ctr5);
      ctr7 = fio___arm_gcm_inc_ctr(ctr6);
      ctr = ctr7;

      /* AES round 0 + GHASH block 0 (init) */
      FIO___ARM_AES_ROUND8(ctr0,
                           ctr1,
                           ctr2,
                           ctr3,
                           ctr4,
                           ctr5,
                           ctr6,
                           ctr7,
                           rk[0]);
      FIO___ARM_GHASH_INIT(gh_lo, gh_hi, gh_mm, prev0_br, htbl[7]);

      /* AES round 1 + GHASH block 1 */
      FIO___ARM_AES_ROUND8(ctr0,
                           ctr1,
                           ctr2,
                           ctr3,
                           ctr4,
                           ctr5,
                           ctr6,
                           ctr7,
                           rk[1]);
      FIO___ARM_GHASH_ACCUM(gh_lo, gh_hi, gh_mm, prev1_br, htbl[6]);

      /* AES round 2 + GHASH block 2 */
      FIO___ARM_AES_ROUND8(ctr0,
                           ctr1,
                           ctr2,
                           ctr3,
                           ctr4,
                           ctr5,
                           ctr6,
                           ctr7,
                           rk[2]);
      FIO___ARM_GHASH_ACCUM(gh_lo, gh_hi, gh_mm, prev2_br, htbl[5]);

      /* AES round 3 + GHASH block 3 */
      FIO___ARM_AES_ROUND8(ctr0,
                           ctr1,
                           ctr2,
                           ctr3,
                           ctr4,
                           ctr5,
                           ctr6,
                           ctr7,
                           rk[3]);
      FIO___ARM_GHASH_ACCUM(gh_lo, gh_hi, gh_mm, prev3_br, htbl[4]);

      /* AES round 4 + GHASH block 4 */
      FIO___ARM_AES_ROUND8(ctr0,
                           ctr1,
                           ctr2,
                           ctr3,
                           ctr4,
                           ctr5,
                           ctr6,
                           ctr7,
                           rk[4]);
      FIO___ARM_GHASH_ACCUM(gh_lo, gh_hi, gh_mm, prev4_br, htbl[3]);

      /* AES round 5 + GHASH block 5 */
      FIO___ARM_AES_ROUND8(ctr0,
                           ctr1,
                           ctr2,
                           ctr3,
                           ctr4,
                           ctr5,
                           ctr6,
                           ctr7,
                           rk[5]);
      FIO___ARM_GHASH_ACCUM(gh_lo, gh_hi, gh_mm, prev5_br, htbl[2]);

      /* AES round 6 + GHASH block 6 */
      FIO___ARM_AES_ROUND8(ctr0,
                           ctr1,
                           ctr2,
                           ctr3,
                           ctr4,
                           ctr5,
                           ctr6,
                           ctr7,
                           rk[6]);
      FIO___ARM_GHASH_ACCUM(gh_lo, gh_hi, gh_mm, prev6_br, htbl[1]);

      /* AES round 7 + GHASH block 7 */
      FIO___ARM_AES_ROUND8(ctr0,
                           ctr1,
                           ctr2,
                           ctr3,
                           ctr4,
                           ctr5,
                           ctr6,
                           ctr7,
                           rk[7]);
      FIO___ARM_GHASH_ACCUM(gh_lo, gh_hi, gh_mm, prev7_br, htbl[0]);

      /* AES round 8 + GHASH finalize */
      FIO___ARM_AES_ROUND8(ctr0,
                           ctr1,
                           ctr2,
                           ctr3,
                           ctr4,
                           ctr5,
                           ctr6,
                           ctr7,
                           rk[8]);
      FIO___ARM_GHASH_FINAL(tag_br, gh_lo, gh_hi, gh_mm);

      /* AES final round */
      FIO___ARM_AES_LAST8(ctr0,
                          ctr1,
                          ctr2,
                          ctr3,
                          ctr4,
                          ctr5,
                          ctr6,
                          ctr7,
                          rk[9],
                          rk[10]);

      /* XOR keystream with plaintext → ciphertext */
      ct0 = veorq_u8(vld1q_u8(p), ctr0);
      ct1 = veorq_u8(vld1q_u8(p + 16), ctr1);
      ct2 = veorq_u8(vld1q_u8(p + 32), ctr2);
      ct3 = veorq_u8(vld1q_u8(p + 48), ctr3);
      ct4 = veorq_u8(vld1q_u8(p + 64), ctr4);
      ct5 = veorq_u8(vld1q_u8(p + 80), ctr5);
      ct6 = veorq_u8(vld1q_u8(p + 96), ctr6);
      ct7 = veorq_u8(vld1q_u8(p + 112), ctr7);

      vst1q_u8(p, ct0);
      vst1q_u8(p + 16, ct1);
      vst1q_u8(p + 32, ct2);
      vst1q_u8(p + 48, ct3);
      vst1q_u8(p + 64, ct4);
      vst1q_u8(p + 80, ct5);
      vst1q_u8(p + 96, ct6);
      vst1q_u8(p + 112, ct7);

      /* Save current ciphertext for GHASH in next iteration */
      prev0_br = veorq_u8(tag_br, vrbitq_u8(ct0));
      prev1_br = vrbitq_u8(ct1);
      prev2_br = vrbitq_u8(ct2);
      prev3_br = vrbitq_u8(ct3);
      prev4_br = vrbitq_u8(ct4);
      prev5_br = vrbitq_u8(ct5);
      prev6_br = vrbitq_u8(ct6);
      prev7_br = vrbitq_u8(ct7);

      p += 128;
      len -= 128;
    }

    /* Epilogue: GHASH the last 8-block batch */
    tag_br = fio___arm_ghash_mult8(prev0_br,
                                   prev1_br,
                                   prev2_br,
                                   prev3_br,
                                   prev4_br,
                                   prev5_br,
                                   prev6_br,
                                   prev7_br,
                                   htbl);
  }

  /* 4-block tail */
  while (len >= 64) {
    uint8x16_t ctr0 = fio___arm_gcm_inc_ctr(ctr);
    uint8x16_t ctr1 = fio___arm_gcm_inc_ctr(ctr0);
    uint8x16_t ctr2 = fio___arm_gcm_inc_ctr(ctr1);
    uint8x16_t ctr3 = fio___arm_gcm_inc_ctr(ctr2);
    ctr = ctr3;

    uint8x16_t ks0 = fio___arm_aes128_encrypt(ctr0, rk);
    uint8x16_t ks1 = fio___arm_aes128_encrypt(ctr1, rk);
    uint8x16_t ks2 = fio___arm_aes128_encrypt(ctr2, rk);
    uint8x16_t ks3 = fio___arm_aes128_encrypt(ctr3, rk);

    uint8x16_t ct0 = veorq_u8(vld1q_u8(p), ks0);
    uint8x16_t ct1 = veorq_u8(vld1q_u8(p + 16), ks1);
    uint8x16_t ct2 = veorq_u8(vld1q_u8(p + 32), ks2);
    uint8x16_t ct3 = veorq_u8(vld1q_u8(p + 48), ks3);

    vst1q_u8(p, ct0);
    vst1q_u8(p + 16, ct1);
    vst1q_u8(p + 32, ct2);
    vst1q_u8(p + 48, ct3);

    uint8x16_t ct0_br = veorq_u8(tag_br, vrbitq_u8(ct0));
    tag_br = fio___arm_ghash_mult4(ct0_br,
                                   vrbitq_u8(ct1),
                                   vrbitq_u8(ct2),
                                   vrbitq_u8(ct3),
                                   htbl);
    p += 64;
    len -= 64;
  }

  /* Single-block tail */
  while (len >= 16) {
    ctr = fio___arm_gcm_inc_ctr(ctr);
    uint8x16_t keystream = fio___arm_aes128_encrypt(ctr, rk);
    uint8x16_t ciphertext = veorq_u8(vld1q_u8(p), keystream);
    vst1q_u8(p, ciphertext);
    tag_br = veorq_u8(tag_br, vrbitq_u8(ciphertext));
    tag_br = fio___arm_ghash_mult1_br(tag_br, htbl);
    p += 16;
    len -= 16;
  }

  /* Handle partial final block */
  if (len > 0) {
    ctr = fio___arm_gcm_inc_ctr(ctr);
    uint8x16_t keystream = fio___arm_aes128_encrypt(ctr, rk);
    uint8_t ks_bytes[16];
    vst1q_u8(ks_bytes, keystream);
    for (size_t i = 0; i < len; ++i)
      p[i] ^= ks_bytes[i];
    uint8_t tmp[16] = {0};
    FIO_MEMCPY(tmp, p, len);
    tag_br = veorq_u8(tag_br, vrbitq_u8(vld1q_u8(tmp)));
    tag_br = fio___arm_ghash_mult1_br(tag_br, htbl);
  }

  /* GHASH length block */
  uint8_t len_block[16] = {0};
  fio_u2buf64_be(len_block, (uint64_t)orig_adlen * 8);
  fio_u2buf64_be(len_block + 8, (uint64_t)orig_len * 8);
  tag_br = veorq_u8(tag_br, vrbitq_u8(vld1q_u8(len_block)));
  tag_br = fio___arm_ghash_mult1_br(tag_br, htbl);

  /* Final tag: convert back from bit-reversed domain */
  uint8x16_t tag = vrbitq_u8(tag_br);
  uint8x16_t s = fio___arm_aes128_encrypt(j0, rk);
  tag = veorq_u8(tag, s);
  vst1q_u8((uint8_t *)mac, tag);

  /* Clear sensitive data */
  fio_secure_zero(rk, sizeof(rk));
  fio_secure_zero(htbl, sizeof(htbl));
  fio_secure_zero(j0_bytes, sizeof(j0_bytes));
}

SFUNC void fio_aes256_gcm_enc(void *restrict mac,
                              void *restrict data,
                              size_t len,
                              const void *ad,
                              size_t adlen,
                              const void *key,
                              const void *nonce) {
  uint8x16_t rk[15];
  uint8x16_t htbl[8], tag_br, ctr, j0;
  uint8_t *p = (uint8_t *)data;
  const uint8_t *aad = (const uint8_t *)ad;
  size_t orig_len = len;
  size_t orig_adlen = adlen;

  fio___arm_aes256_key_expand(rk, (const uint8_t *)key);
  uint8x16_t zero = vdupq_n_u8(0);
  uint8x16_t h = fio___arm_aes256_encrypt(zero, rk);

  /* Precompute H powers: H⁸ for 8-block, H⁴ for 4-block, H¹ for small */
  if (len >= 128 || adlen >= 128)
    fio___arm_ghash_precompute(h, htbl, 1);
  else if (len >= 64 || adlen >= 64)
    fio___arm_ghash_precompute(h, htbl, 0);
  else
    htbl[0] = vrbitq_u8(h);

  uint8_t j0_bytes[16] = {0};
  FIO_MEMCPY(j0_bytes, nonce, 12);
  j0_bytes[15] = 1;
  j0 = vld1q_u8(j0_bytes);
  ctr = j0;
  tag_br = vdupq_n_u8(0);

  /* GHASH over AAD - 8-block, 4-block, single */
  while (adlen >= 128) {
    uint8x16_t a0 = veorq_u8(tag_br, vrbitq_u8(vld1q_u8(aad)));
    uint8x16_t a1 = vrbitq_u8(vld1q_u8(aad + 16));
    uint8x16_t a2 = vrbitq_u8(vld1q_u8(aad + 32));
    uint8x16_t a3 = vrbitq_u8(vld1q_u8(aad + 48));
    uint8x16_t a4 = vrbitq_u8(vld1q_u8(aad + 64));
    uint8x16_t a5 = vrbitq_u8(vld1q_u8(aad + 80));
    uint8x16_t a6 = vrbitq_u8(vld1q_u8(aad + 96));
    uint8x16_t a7 = vrbitq_u8(vld1q_u8(aad + 112));
    tag_br = fio___arm_ghash_mult8(a0, a1, a2, a3, a4, a5, a6, a7, htbl);
    aad += 128;
    adlen -= 128;
  }
  while (adlen >= 64) {
    uint8x16_t a0 = veorq_u8(tag_br, vrbitq_u8(vld1q_u8(aad)));
    uint8x16_t a1 = vrbitq_u8(vld1q_u8(aad + 16));
    uint8x16_t a2 = vrbitq_u8(vld1q_u8(aad + 32));
    uint8x16_t a3 = vrbitq_u8(vld1q_u8(aad + 48));
    tag_br = fio___arm_ghash_mult4(a0, a1, a2, a3, htbl);
    aad += 64;
    adlen -= 64;
  }
  while (adlen >= 16) {
    tag_br = veorq_u8(tag_br, vrbitq_u8(vld1q_u8(aad)));
    tag_br = fio___arm_ghash_mult1_br(tag_br, htbl);
    aad += 16;
    adlen -= 16;
  }
  if (adlen > 0) {
    uint8_t tmp[16] = {0};
    FIO_MEMCPY(tmp, aad, adlen);
    tag_br = veorq_u8(tag_br, vrbitq_u8(vld1q_u8(tmp)));
    tag_br = fio___arm_ghash_mult1_br(tag_br, htbl);
  }

  /* === 8-block interleaved AES-CTR encryption + GHASH === */
  if (len >= 256) {
    /* Prologue: encrypt first 8 blocks */
    uint8x16_t ctr0 = fio___arm_gcm_inc_ctr(ctr);
    uint8x16_t ctr1 = fio___arm_gcm_inc_ctr(ctr0);
    uint8x16_t ctr2 = fio___arm_gcm_inc_ctr(ctr1);
    uint8x16_t ctr3 = fio___arm_gcm_inc_ctr(ctr2);
    uint8x16_t ctr4 = fio___arm_gcm_inc_ctr(ctr3);
    uint8x16_t ctr5 = fio___arm_gcm_inc_ctr(ctr4);
    uint8x16_t ctr6 = fio___arm_gcm_inc_ctr(ctr5);
    uint8x16_t ctr7 = fio___arm_gcm_inc_ctr(ctr6);
    ctr = ctr7;

    uint8x16_t ct0 = veorq_u8(vld1q_u8(p), fio___arm_aes256_encrypt(ctr0, rk));
    uint8x16_t ct1 =
        veorq_u8(vld1q_u8(p + 16), fio___arm_aes256_encrypt(ctr1, rk));
    uint8x16_t ct2 =
        veorq_u8(vld1q_u8(p + 32), fio___arm_aes256_encrypt(ctr2, rk));
    uint8x16_t ct3 =
        veorq_u8(vld1q_u8(p + 48), fio___arm_aes256_encrypt(ctr3, rk));
    uint8x16_t ct4 =
        veorq_u8(vld1q_u8(p + 64), fio___arm_aes256_encrypt(ctr4, rk));
    uint8x16_t ct5 =
        veorq_u8(vld1q_u8(p + 80), fio___arm_aes256_encrypt(ctr5, rk));
    uint8x16_t ct6 =
        veorq_u8(vld1q_u8(p + 96), fio___arm_aes256_encrypt(ctr6, rk));
    uint8x16_t ct7 =
        veorq_u8(vld1q_u8(p + 112), fio___arm_aes256_encrypt(ctr7, rk));

    vst1q_u8(p, ct0);
    vst1q_u8(p + 16, ct1);
    vst1q_u8(p + 32, ct2);
    vst1q_u8(p + 48, ct3);
    vst1q_u8(p + 64, ct4);
    vst1q_u8(p + 80, ct5);
    vst1q_u8(p + 96, ct6);
    vst1q_u8(p + 112, ct7);

    uint8x16_t prev0_br = veorq_u8(tag_br, vrbitq_u8(ct0));
    uint8x16_t prev1_br = vrbitq_u8(ct1);
    uint8x16_t prev2_br = vrbitq_u8(ct2);
    uint8x16_t prev3_br = vrbitq_u8(ct3);
    uint8x16_t prev4_br = vrbitq_u8(ct4);
    uint8x16_t prev5_br = vrbitq_u8(ct5);
    uint8x16_t prev6_br = vrbitq_u8(ct6);
    uint8x16_t prev7_br = vrbitq_u8(ct7);

    p += 128;
    len -= 128;

    /* Steady state: interleaved AES-256 encrypt + GHASH */
    while (len >= 128) {
      uint64x2_t gh_lo, gh_hi, gh_mm;

      ctr0 = fio___arm_gcm_inc_ctr(ctr);
      ctr1 = fio___arm_gcm_inc_ctr(ctr0);
      ctr2 = fio___arm_gcm_inc_ctr(ctr1);
      ctr3 = fio___arm_gcm_inc_ctr(ctr2);
      ctr4 = fio___arm_gcm_inc_ctr(ctr3);
      ctr5 = fio___arm_gcm_inc_ctr(ctr4);
      ctr6 = fio___arm_gcm_inc_ctr(ctr5);
      ctr7 = fio___arm_gcm_inc_ctr(ctr6);
      ctr = ctr7;

      /* AES rounds 0-7 interleaved with GHASH blocks 0-7 */
      FIO___ARM_AES_ROUND8(ctr0,
                           ctr1,
                           ctr2,
                           ctr3,
                           ctr4,
                           ctr5,
                           ctr6,
                           ctr7,
                           rk[0]);
      FIO___ARM_GHASH_INIT(gh_lo, gh_hi, gh_mm, prev0_br, htbl[7]);

      FIO___ARM_AES_ROUND8(ctr0,
                           ctr1,
                           ctr2,
                           ctr3,
                           ctr4,
                           ctr5,
                           ctr6,
                           ctr7,
                           rk[1]);
      FIO___ARM_GHASH_ACCUM(gh_lo, gh_hi, gh_mm, prev1_br, htbl[6]);

      FIO___ARM_AES_ROUND8(ctr0,
                           ctr1,
                           ctr2,
                           ctr3,
                           ctr4,
                           ctr5,
                           ctr6,
                           ctr7,
                           rk[2]);
      FIO___ARM_GHASH_ACCUM(gh_lo, gh_hi, gh_mm, prev2_br, htbl[5]);

      FIO___ARM_AES_ROUND8(ctr0,
                           ctr1,
                           ctr2,
                           ctr3,
                           ctr4,
                           ctr5,
                           ctr6,
                           ctr7,
                           rk[3]);
      FIO___ARM_GHASH_ACCUM(gh_lo, gh_hi, gh_mm, prev3_br, htbl[4]);

      FIO___ARM_AES_ROUND8(ctr0,
                           ctr1,
                           ctr2,
                           ctr3,
                           ctr4,
                           ctr5,
                           ctr6,
                           ctr7,
                           rk[4]);
      FIO___ARM_GHASH_ACCUM(gh_lo, gh_hi, gh_mm, prev4_br, htbl[3]);

      FIO___ARM_AES_ROUND8(ctr0,
                           ctr1,
                           ctr2,
                           ctr3,
                           ctr4,
                           ctr5,
                           ctr6,
                           ctr7,
                           rk[5]);
      FIO___ARM_GHASH_ACCUM(gh_lo, gh_hi, gh_mm, prev5_br, htbl[2]);

      FIO___ARM_AES_ROUND8(ctr0,
                           ctr1,
                           ctr2,
                           ctr3,
                           ctr4,
                           ctr5,
                           ctr6,
                           ctr7,
                           rk[6]);
      FIO___ARM_GHASH_ACCUM(gh_lo, gh_hi, gh_mm, prev6_br, htbl[1]);

      FIO___ARM_AES_ROUND8(ctr0,
                           ctr1,
                           ctr2,
                           ctr3,
                           ctr4,
                           ctr5,
                           ctr6,
                           ctr7,
                           rk[7]);
      FIO___ARM_GHASH_ACCUM(gh_lo, gh_hi, gh_mm, prev7_br, htbl[0]);

      /* AES rounds 8-12 + GHASH finalize */
      FIO___ARM_AES_ROUND8(ctr0,
                           ctr1,
                           ctr2,
                           ctr3,
                           ctr4,
                           ctr5,
                           ctr6,
                           ctr7,
                           rk[8]);
      FIO___ARM_GHASH_FINAL(tag_br, gh_lo, gh_hi, gh_mm);

      FIO___ARM_AES_ROUND8(ctr0,
                           ctr1,
                           ctr2,
                           ctr3,
                           ctr4,
                           ctr5,
                           ctr6,
                           ctr7,
                           rk[9]);
      FIO___ARM_AES_ROUND8(ctr0,
                           ctr1,
                           ctr2,
                           ctr3,
                           ctr4,
                           ctr5,
                           ctr6,
                           ctr7,
                           rk[10]);
      FIO___ARM_AES_ROUND8(ctr0,
                           ctr1,
                           ctr2,
                           ctr3,
                           ctr4,
                           ctr5,
                           ctr6,
                           ctr7,
                           rk[11]);
      FIO___ARM_AES_ROUND8(ctr0,
                           ctr1,
                           ctr2,
                           ctr3,
                           ctr4,
                           ctr5,
                           ctr6,
                           ctr7,
                           rk[12]);
      FIO___ARM_AES_LAST8(ctr0,
                          ctr1,
                          ctr2,
                          ctr3,
                          ctr4,
                          ctr5,
                          ctr6,
                          ctr7,
                          rk[13],
                          rk[14]);

      ct0 = veorq_u8(vld1q_u8(p), ctr0);
      ct1 = veorq_u8(vld1q_u8(p + 16), ctr1);
      ct2 = veorq_u8(vld1q_u8(p + 32), ctr2);
      ct3 = veorq_u8(vld1q_u8(p + 48), ctr3);
      ct4 = veorq_u8(vld1q_u8(p + 64), ctr4);
      ct5 = veorq_u8(vld1q_u8(p + 80), ctr5);
      ct6 = veorq_u8(vld1q_u8(p + 96), ctr6);
      ct7 = veorq_u8(vld1q_u8(p + 112), ctr7);

      vst1q_u8(p, ct0);
      vst1q_u8(p + 16, ct1);
      vst1q_u8(p + 32, ct2);
      vst1q_u8(p + 48, ct3);
      vst1q_u8(p + 64, ct4);
      vst1q_u8(p + 80, ct5);
      vst1q_u8(p + 96, ct6);
      vst1q_u8(p + 112, ct7);

      prev0_br = veorq_u8(tag_br, vrbitq_u8(ct0));
      prev1_br = vrbitq_u8(ct1);
      prev2_br = vrbitq_u8(ct2);
      prev3_br = vrbitq_u8(ct3);
      prev4_br = vrbitq_u8(ct4);
      prev5_br = vrbitq_u8(ct5);
      prev6_br = vrbitq_u8(ct6);
      prev7_br = vrbitq_u8(ct7);

      p += 128;
      len -= 128;
    }

    /* Epilogue: GHASH the last 8-block batch */
    tag_br = fio___arm_ghash_mult8(prev0_br,
                                   prev1_br,
                                   prev2_br,
                                   prev3_br,
                                   prev4_br,
                                   prev5_br,
                                   prev6_br,
                                   prev7_br,
                                   htbl);
  }

  /* 4-block tail */
  while (len >= 64) {
    uint8x16_t ctr0 = fio___arm_gcm_inc_ctr(ctr);
    uint8x16_t ctr1 = fio___arm_gcm_inc_ctr(ctr0);
    uint8x16_t ctr2 = fio___arm_gcm_inc_ctr(ctr1);
    uint8x16_t ctr3 = fio___arm_gcm_inc_ctr(ctr2);
    ctr = ctr3;

    uint8x16_t ks0 = fio___arm_aes256_encrypt(ctr0, rk);
    uint8x16_t ks1 = fio___arm_aes256_encrypt(ctr1, rk);
    uint8x16_t ks2 = fio___arm_aes256_encrypt(ctr2, rk);
    uint8x16_t ks3 = fio___arm_aes256_encrypt(ctr3, rk);

    uint8x16_t ct0 = veorq_u8(vld1q_u8(p), ks0);
    uint8x16_t ct1 = veorq_u8(vld1q_u8(p + 16), ks1);
    uint8x16_t ct2 = veorq_u8(vld1q_u8(p + 32), ks2);
    uint8x16_t ct3 = veorq_u8(vld1q_u8(p + 48), ks3);

    vst1q_u8(p, ct0);
    vst1q_u8(p + 16, ct1);
    vst1q_u8(p + 32, ct2);
    vst1q_u8(p + 48, ct3);

    uint8x16_t ct0_br = veorq_u8(tag_br, vrbitq_u8(ct0));
    tag_br = fio___arm_ghash_mult4(ct0_br,
                                   vrbitq_u8(ct1),
                                   vrbitq_u8(ct2),
                                   vrbitq_u8(ct3),
                                   htbl);
    p += 64;
    len -= 64;
  }

  /* Single-block tail */
  while (len >= 16) {
    ctr = fio___arm_gcm_inc_ctr(ctr);
    uint8x16_t ciphertext =
        veorq_u8(vld1q_u8(p), fio___arm_aes256_encrypt(ctr, rk));
    vst1q_u8(p, ciphertext);
    tag_br = veorq_u8(tag_br, vrbitq_u8(ciphertext));
    tag_br = fio___arm_ghash_mult1_br(tag_br, htbl);
    p += 16;
    len -= 16;
  }

  if (len > 0) {
    ctr = fio___arm_gcm_inc_ctr(ctr);
    uint8x16_t keystream = fio___arm_aes256_encrypt(ctr, rk);
    uint8_t ks_bytes[16];
    vst1q_u8(ks_bytes, keystream);
    for (size_t i = 0; i < len; ++i)
      p[i] ^= ks_bytes[i];
    uint8_t tmp[16] = {0};
    FIO_MEMCPY(tmp, p, len);
    tag_br = veorq_u8(tag_br, vrbitq_u8(vld1q_u8(tmp)));
    tag_br = fio___arm_ghash_mult1_br(tag_br, htbl);
  }

  /* GHASH length block */
  uint8_t len_block[16] = {0};
  fio_u2buf64_be(len_block, (uint64_t)orig_adlen * 8);
  fio_u2buf64_be(len_block + 8, (uint64_t)orig_len * 8);
  tag_br = veorq_u8(tag_br, vrbitq_u8(vld1q_u8(len_block)));
  tag_br = fio___arm_ghash_mult1_br(tag_br, htbl);

  /* Final tag: convert back from bit-reversed domain */
  uint8x16_t tag = vrbitq_u8(tag_br);
  uint8x16_t s = fio___arm_aes256_encrypt(j0, rk);
  tag = veorq_u8(tag, s);
  vst1q_u8((uint8_t *)mac, tag);

  fio_secure_zero(rk, sizeof(rk));
  fio_secure_zero(htbl, sizeof(htbl));
  fio_secure_zero(j0_bytes, sizeof(j0_bytes));
}

SFUNC int fio_aes128_gcm_dec(void *restrict mac,
                             void *restrict data,
                             size_t len,
                             const void *ad,
                             size_t adlen,
                             const void *key,
                             const void *nonce) {
  uint8x16_t rk[11];
  uint8x16_t htbl[8], tag_br, ctr, j0;
  uint8_t *p = (uint8_t *)data;
  const uint8_t *aad = (const uint8_t *)ad;
  size_t orig_len = len;
  size_t orig_adlen = adlen;

  fio___arm_aes128_key_expand(rk, (const uint8_t *)key);
  uint8x16_t zero = vdupq_n_u8(0);
  uint8x16_t h = fio___arm_aes128_encrypt(zero, rk);

  /* Precompute H powers: H⁸ for 8-block, H⁴ for 4-block, H¹ for small */
  if (len >= 128 || adlen >= 128)
    fio___arm_ghash_precompute(h, htbl, 1);
  else if (len >= 64 || adlen >= 64)
    fio___arm_ghash_precompute(h, htbl, 0);
  else
    htbl[0] = vrbitq_u8(h);

  uint8_t j0_bytes[16] = {0};
  FIO_MEMCPY(j0_bytes, nonce, 12);
  j0_bytes[15] = 1;
  j0 = vld1q_u8(j0_bytes);
  ctr = j0;
  tag_br = vdupq_n_u8(0);

  /* GHASH over AAD - 8-block, 4-block, single */
  while (adlen >= 128) {
    uint8x16_t a0 = veorq_u8(tag_br, vrbitq_u8(vld1q_u8(aad)));
    uint8x16_t a1 = vrbitq_u8(vld1q_u8(aad + 16));
    uint8x16_t a2 = vrbitq_u8(vld1q_u8(aad + 32));
    uint8x16_t a3 = vrbitq_u8(vld1q_u8(aad + 48));
    uint8x16_t a4 = vrbitq_u8(vld1q_u8(aad + 64));
    uint8x16_t a5 = vrbitq_u8(vld1q_u8(aad + 80));
    uint8x16_t a6 = vrbitq_u8(vld1q_u8(aad + 96));
    uint8x16_t a7 = vrbitq_u8(vld1q_u8(aad + 112));
    tag_br = fio___arm_ghash_mult8(a0, a1, a2, a3, a4, a5, a6, a7, htbl);
    aad += 128;
    adlen -= 128;
  }
  while (adlen >= 64) {
    uint8x16_t a0 = veorq_u8(tag_br, vrbitq_u8(vld1q_u8(aad)));
    uint8x16_t a1 = vrbitq_u8(vld1q_u8(aad + 16));
    uint8x16_t a2 = vrbitq_u8(vld1q_u8(aad + 32));
    uint8x16_t a3 = vrbitq_u8(vld1q_u8(aad + 48));
    tag_br = fio___arm_ghash_mult4(a0, a1, a2, a3, htbl);
    aad += 64;
    adlen -= 64;
  }
  while (adlen >= 16) {
    tag_br = veorq_u8(tag_br, vrbitq_u8(vld1q_u8(aad)));
    tag_br = fio___arm_ghash_mult1_br(tag_br, htbl);
    aad += 16;
    adlen -= 16;
  }
  if (adlen > 0) {
    uint8_t tmp[16] = {0};
    FIO_MEMCPY(tmp, aad, adlen);
    tag_br = veorq_u8(tag_br, vrbitq_u8(vld1q_u8(tmp)));
    tag_br = fio___arm_ghash_mult1_br(tag_br, htbl);
  }

  /* GHASH over ciphertext — 8-block, 4-block, single.
   * For decryption, GHASH operates on ciphertext (available before decrypt).
   * We interleave GHASH with AES-CTR keystream generation. */
  const uint8_t *ct = p;
  size_t ct_len = orig_len;
  while (ct_len >= 128) {
    uint8x16_t c0 = veorq_u8(tag_br, vrbitq_u8(vld1q_u8(ct)));
    uint8x16_t c1 = vrbitq_u8(vld1q_u8(ct + 16));
    uint8x16_t c2 = vrbitq_u8(vld1q_u8(ct + 32));
    uint8x16_t c3 = vrbitq_u8(vld1q_u8(ct + 48));
    uint8x16_t c4 = vrbitq_u8(vld1q_u8(ct + 64));
    uint8x16_t c5 = vrbitq_u8(vld1q_u8(ct + 80));
    uint8x16_t c6 = vrbitq_u8(vld1q_u8(ct + 96));
    uint8x16_t c7 = vrbitq_u8(vld1q_u8(ct + 112));
    tag_br = fio___arm_ghash_mult8(c0, c1, c2, c3, c4, c5, c6, c7, htbl);
    ct += 128;
    ct_len -= 128;
  }
  while (ct_len >= 64) {
    uint8x16_t c0 = veorq_u8(tag_br, vrbitq_u8(vld1q_u8(ct)));
    uint8x16_t c1 = vrbitq_u8(vld1q_u8(ct + 16));
    uint8x16_t c2 = vrbitq_u8(vld1q_u8(ct + 32));
    uint8x16_t c3 = vrbitq_u8(vld1q_u8(ct + 48));
    tag_br = fio___arm_ghash_mult4(c0, c1, c2, c3, htbl);
    ct += 64;
    ct_len -= 64;
  }
  while (ct_len >= 16) {
    tag_br = veorq_u8(tag_br, vrbitq_u8(vld1q_u8(ct)));
    tag_br = fio___arm_ghash_mult1_br(tag_br, htbl);
    ct += 16;
    ct_len -= 16;
  }
  if (ct_len > 0) {
    uint8_t tmp[16] = {0};
    FIO_MEMCPY(tmp, ct, ct_len);
    tag_br = veorq_u8(tag_br, vrbitq_u8(vld1q_u8(tmp)));
    tag_br = fio___arm_ghash_mult1_br(tag_br, htbl);
  }

  /* GHASH length block */
  uint8_t len_block[16] = {0};
  fio_u2buf64_be(len_block, (uint64_t)orig_adlen * 8);
  fio_u2buf64_be(len_block + 8, (uint64_t)orig_len * 8);
  tag_br = veorq_u8(tag_br, vrbitq_u8(vld1q_u8(len_block)));
  tag_br = fio___arm_ghash_mult1_br(tag_br, htbl);

  /* Compute and verify tag */
  uint8x16_t tag = vrbitq_u8(tag_br);
  uint8x16_t s = fio___arm_aes128_encrypt(j0, rk);
  tag = veorq_u8(tag, s);
  uint8_t computed_mac[16];
  vst1q_u8(computed_mac, tag);
  if (!fio_ct_is_eq(computed_mac, mac, 16)) {
    fio_secure_zero(computed_mac, sizeof(computed_mac));
    fio_secure_zero(rk, sizeof(rk));
    fio_secure_zero(htbl, sizeof(htbl));
    fio_secure_zero(j0_bytes, sizeof(j0_bytes));
    return -1;
  }
  fio_secure_zero(computed_mac, sizeof(computed_mac));

  /* Decrypt — 8-block, 4-block, single-block, partial */
  while (len >= 128) {
    uint8x16_t ctr0 = fio___arm_gcm_inc_ctr(ctr);
    uint8x16_t ctr1 = fio___arm_gcm_inc_ctr(ctr0);
    uint8x16_t ctr2 = fio___arm_gcm_inc_ctr(ctr1);
    uint8x16_t ctr3 = fio___arm_gcm_inc_ctr(ctr2);
    uint8x16_t ctr4 = fio___arm_gcm_inc_ctr(ctr3);
    uint8x16_t ctr5 = fio___arm_gcm_inc_ctr(ctr4);
    uint8x16_t ctr6 = fio___arm_gcm_inc_ctr(ctr5);
    uint8x16_t ctr7 = fio___arm_gcm_inc_ctr(ctr6);
    ctr = ctr7;

    vst1q_u8(p, veorq_u8(vld1q_u8(p), fio___arm_aes128_encrypt(ctr0, rk)));
    vst1q_u8(p + 16,
             veorq_u8(vld1q_u8(p + 16), fio___arm_aes128_encrypt(ctr1, rk)));
    vst1q_u8(p + 32,
             veorq_u8(vld1q_u8(p + 32), fio___arm_aes128_encrypt(ctr2, rk)));
    vst1q_u8(p + 48,
             veorq_u8(vld1q_u8(p + 48), fio___arm_aes128_encrypt(ctr3, rk)));
    vst1q_u8(p + 64,
             veorq_u8(vld1q_u8(p + 64), fio___arm_aes128_encrypt(ctr4, rk)));
    vst1q_u8(p + 80,
             veorq_u8(vld1q_u8(p + 80), fio___arm_aes128_encrypt(ctr5, rk)));
    vst1q_u8(p + 96,
             veorq_u8(vld1q_u8(p + 96), fio___arm_aes128_encrypt(ctr6, rk)));
    vst1q_u8(p + 112,
             veorq_u8(vld1q_u8(p + 112), fio___arm_aes128_encrypt(ctr7, rk)));

    p += 128;
    len -= 128;
  }
  while (len >= 64) {
    uint8x16_t ctr0 = fio___arm_gcm_inc_ctr(ctr);
    uint8x16_t ctr1 = fio___arm_gcm_inc_ctr(ctr0);
    uint8x16_t ctr2 = fio___arm_gcm_inc_ctr(ctr1);
    uint8x16_t ctr3 = fio___arm_gcm_inc_ctr(ctr2);
    ctr = ctr3;

    vst1q_u8(p, veorq_u8(vld1q_u8(p), fio___arm_aes128_encrypt(ctr0, rk)));
    vst1q_u8(p + 16,
             veorq_u8(vld1q_u8(p + 16), fio___arm_aes128_encrypt(ctr1, rk)));
    vst1q_u8(p + 32,
             veorq_u8(vld1q_u8(p + 32), fio___arm_aes128_encrypt(ctr2, rk)));
    vst1q_u8(p + 48,
             veorq_u8(vld1q_u8(p + 48), fio___arm_aes128_encrypt(ctr3, rk)));

    p += 64;
    len -= 64;
  }
  while (len >= 16) {
    ctr = fio___arm_gcm_inc_ctr(ctr);
    vst1q_u8(p, veorq_u8(vld1q_u8(p), fio___arm_aes128_encrypt(ctr, rk)));
    p += 16;
    len -= 16;
  }
  if (len > 0) {
    ctr = fio___arm_gcm_inc_ctr(ctr);
    uint8_t ks_bytes[16];
    vst1q_u8(ks_bytes, fio___arm_aes128_encrypt(ctr, rk));
    for (size_t i = 0; i < len; ++i)
      p[i] ^= ks_bytes[i];
  }

  fio_secure_zero(rk, sizeof(rk));
  fio_secure_zero(htbl, sizeof(htbl));
  fio_secure_zero(j0_bytes, sizeof(j0_bytes));
  return 0;
}

SFUNC int fio_aes256_gcm_dec(void *restrict mac,
                             void *restrict data,
                             size_t len,
                             const void *ad,
                             size_t adlen,
                             const void *key,
                             const void *nonce) {
  uint8x16_t rk[15];
  uint8x16_t htbl[8], tag_br, ctr, j0;
  uint8_t *p = (uint8_t *)data;
  const uint8_t *aad = (const uint8_t *)ad;
  size_t orig_len = len;
  size_t orig_adlen = adlen;

  fio___arm_aes256_key_expand(rk, (const uint8_t *)key);
  uint8x16_t zero = vdupq_n_u8(0);
  uint8x16_t h = fio___arm_aes256_encrypt(zero, rk);

  /* Precompute H powers: H⁸ for 8-block, H⁴ for 4-block, H¹ for small */
  if (len >= 128 || adlen >= 128)
    fio___arm_ghash_precompute(h, htbl, 1);
  else if (len >= 64 || adlen >= 64)
    fio___arm_ghash_precompute(h, htbl, 0);
  else
    htbl[0] = vrbitq_u8(h);

  uint8_t j0_bytes[16] = {0};
  FIO_MEMCPY(j0_bytes, nonce, 12);
  j0_bytes[15] = 1;
  j0 = vld1q_u8(j0_bytes);
  ctr = j0;
  tag_br = vdupq_n_u8(0);

  /* GHASH over AAD - 8-block, 4-block, single */
  while (adlen >= 128) {
    uint8x16_t a0 = veorq_u8(tag_br, vrbitq_u8(vld1q_u8(aad)));
    uint8x16_t a1 = vrbitq_u8(vld1q_u8(aad + 16));
    uint8x16_t a2 = vrbitq_u8(vld1q_u8(aad + 32));
    uint8x16_t a3 = vrbitq_u8(vld1q_u8(aad + 48));
    uint8x16_t a4 = vrbitq_u8(vld1q_u8(aad + 64));
    uint8x16_t a5 = vrbitq_u8(vld1q_u8(aad + 80));
    uint8x16_t a6 = vrbitq_u8(vld1q_u8(aad + 96));
    uint8x16_t a7 = vrbitq_u8(vld1q_u8(aad + 112));
    tag_br = fio___arm_ghash_mult8(a0, a1, a2, a3, a4, a5, a6, a7, htbl);
    aad += 128;
    adlen -= 128;
  }
  while (adlen >= 64) {
    uint8x16_t a0 = veorq_u8(tag_br, vrbitq_u8(vld1q_u8(aad)));
    uint8x16_t a1 = vrbitq_u8(vld1q_u8(aad + 16));
    uint8x16_t a2 = vrbitq_u8(vld1q_u8(aad + 32));
    uint8x16_t a3 = vrbitq_u8(vld1q_u8(aad + 48));
    tag_br = fio___arm_ghash_mult4(a0, a1, a2, a3, htbl);
    aad += 64;
    adlen -= 64;
  }
  while (adlen >= 16) {
    tag_br = veorq_u8(tag_br, vrbitq_u8(vld1q_u8(aad)));
    tag_br = fio___arm_ghash_mult1_br(tag_br, htbl);
    aad += 16;
    adlen -= 16;
  }
  if (adlen > 0) {
    uint8_t tmp[16] = {0};
    FIO_MEMCPY(tmp, aad, adlen);
    tag_br = veorq_u8(tag_br, vrbitq_u8(vld1q_u8(tmp)));
    tag_br = fio___arm_ghash_mult1_br(tag_br, htbl);
  }

  /* GHASH over ciphertext — 8-block, 4-block, single */
  const uint8_t *ct = p;
  size_t ct_len = orig_len;
  while (ct_len >= 128) {
    uint8x16_t c0 = veorq_u8(tag_br, vrbitq_u8(vld1q_u8(ct)));
    uint8x16_t c1 = vrbitq_u8(vld1q_u8(ct + 16));
    uint8x16_t c2 = vrbitq_u8(vld1q_u8(ct + 32));
    uint8x16_t c3 = vrbitq_u8(vld1q_u8(ct + 48));
    uint8x16_t c4 = vrbitq_u8(vld1q_u8(ct + 64));
    uint8x16_t c5 = vrbitq_u8(vld1q_u8(ct + 80));
    uint8x16_t c6 = vrbitq_u8(vld1q_u8(ct + 96));
    uint8x16_t c7 = vrbitq_u8(vld1q_u8(ct + 112));
    tag_br = fio___arm_ghash_mult8(c0, c1, c2, c3, c4, c5, c6, c7, htbl);
    ct += 128;
    ct_len -= 128;
  }
  while (ct_len >= 64) {
    uint8x16_t c0 = veorq_u8(tag_br, vrbitq_u8(vld1q_u8(ct)));
    uint8x16_t c1 = vrbitq_u8(vld1q_u8(ct + 16));
    uint8x16_t c2 = vrbitq_u8(vld1q_u8(ct + 32));
    uint8x16_t c3 = vrbitq_u8(vld1q_u8(ct + 48));
    tag_br = fio___arm_ghash_mult4(c0, c1, c2, c3, htbl);
    ct += 64;
    ct_len -= 64;
  }
  while (ct_len >= 16) {
    tag_br = veorq_u8(tag_br, vrbitq_u8(vld1q_u8(ct)));
    tag_br = fio___arm_ghash_mult1_br(tag_br, htbl);
    ct += 16;
    ct_len -= 16;
  }
  if (ct_len > 0) {
    uint8_t tmp[16] = {0};
    FIO_MEMCPY(tmp, ct, ct_len);
    tag_br = veorq_u8(tag_br, vrbitq_u8(vld1q_u8(tmp)));
    tag_br = fio___arm_ghash_mult1_br(tag_br, htbl);
  }

  /* GHASH length block */
  uint8_t len_block[16] = {0};
  fio_u2buf64_be(len_block, (uint64_t)orig_adlen * 8);
  fio_u2buf64_be(len_block + 8, (uint64_t)orig_len * 8);
  tag_br = veorq_u8(tag_br, vrbitq_u8(vld1q_u8(len_block)));
  tag_br = fio___arm_ghash_mult1_br(tag_br, htbl);

  /* Compute and verify tag */
  uint8x16_t tag = vrbitq_u8(tag_br);
  uint8x16_t s = fio___arm_aes256_encrypt(j0, rk);
  tag = veorq_u8(tag, s);
  uint8_t computed_mac[16];
  vst1q_u8(computed_mac, tag);
  if (!fio_ct_is_eq(computed_mac, mac, 16)) {
    fio_secure_zero(computed_mac, sizeof(computed_mac));
    fio_secure_zero(rk, sizeof(rk));
    fio_secure_zero(htbl, sizeof(htbl));
    fio_secure_zero(j0_bytes, sizeof(j0_bytes));
    return -1;
  }
  fio_secure_zero(computed_mac, sizeof(computed_mac));

  /* Decrypt — 8-block, 4-block, single-block, partial */
  while (len >= 128) {
    uint8x16_t ctr0 = fio___arm_gcm_inc_ctr(ctr);
    uint8x16_t ctr1 = fio___arm_gcm_inc_ctr(ctr0);
    uint8x16_t ctr2 = fio___arm_gcm_inc_ctr(ctr1);
    uint8x16_t ctr3 = fio___arm_gcm_inc_ctr(ctr2);
    uint8x16_t ctr4 = fio___arm_gcm_inc_ctr(ctr3);
    uint8x16_t ctr5 = fio___arm_gcm_inc_ctr(ctr4);
    uint8x16_t ctr6 = fio___arm_gcm_inc_ctr(ctr5);
    uint8x16_t ctr7 = fio___arm_gcm_inc_ctr(ctr6);
    ctr = ctr7;

    vst1q_u8(p, veorq_u8(vld1q_u8(p), fio___arm_aes256_encrypt(ctr0, rk)));
    vst1q_u8(p + 16,
             veorq_u8(vld1q_u8(p + 16), fio___arm_aes256_encrypt(ctr1, rk)));
    vst1q_u8(p + 32,
             veorq_u8(vld1q_u8(p + 32), fio___arm_aes256_encrypt(ctr2, rk)));
    vst1q_u8(p + 48,
             veorq_u8(vld1q_u8(p + 48), fio___arm_aes256_encrypt(ctr3, rk)));
    vst1q_u8(p + 64,
             veorq_u8(vld1q_u8(p + 64), fio___arm_aes256_encrypt(ctr4, rk)));
    vst1q_u8(p + 80,
             veorq_u8(vld1q_u8(p + 80), fio___arm_aes256_encrypt(ctr5, rk)));
    vst1q_u8(p + 96,
             veorq_u8(vld1q_u8(p + 96), fio___arm_aes256_encrypt(ctr6, rk)));
    vst1q_u8(p + 112,
             veorq_u8(vld1q_u8(p + 112), fio___arm_aes256_encrypt(ctr7, rk)));

    p += 128;
    len -= 128;
  }
  while (len >= 64) {
    uint8x16_t ctr0 = fio___arm_gcm_inc_ctr(ctr);
    uint8x16_t ctr1 = fio___arm_gcm_inc_ctr(ctr0);
    uint8x16_t ctr2 = fio___arm_gcm_inc_ctr(ctr1);
    uint8x16_t ctr3 = fio___arm_gcm_inc_ctr(ctr2);
    ctr = ctr3;

    vst1q_u8(p, veorq_u8(vld1q_u8(p), fio___arm_aes256_encrypt(ctr0, rk)));
    vst1q_u8(p + 16,
             veorq_u8(vld1q_u8(p + 16), fio___arm_aes256_encrypt(ctr1, rk)));
    vst1q_u8(p + 32,
             veorq_u8(vld1q_u8(p + 32), fio___arm_aes256_encrypt(ctr2, rk)));
    vst1q_u8(p + 48,
             veorq_u8(vld1q_u8(p + 48), fio___arm_aes256_encrypt(ctr3, rk)));

    p += 64;
    len -= 64;
  }
  while (len >= 16) {
    ctr = fio___arm_gcm_inc_ctr(ctr);
    vst1q_u8(p, veorq_u8(vld1q_u8(p), fio___arm_aes256_encrypt(ctr, rk)));
    p += 16;
    len -= 16;
  }
  if (len > 0) {
    ctr = fio___arm_gcm_inc_ctr(ctr);
    uint8_t ks_bytes[16];
    vst1q_u8(ks_bytes, fio___arm_aes256_encrypt(ctr, rk));
    for (size_t i = 0; i < len; ++i)
      p[i] ^= ks_bytes[i];
  }

  fio_secure_zero(rk, sizeof(rk));
  fio_secure_zero(htbl, sizeof(htbl));
  fio_secure_zero(j0_bytes, sizeof(j0_bytes));
  return 0;
}

/* *****************************************************************************
Portable (Software) Implementation - Fallback
***************************************************************************** */
#else /* No hardware acceleration */

/* *****************************************************************************
Bitsliced AES Helper Functions (Constant-Time Software Fallback)

Based on BearSSL ct64 (Thomas Pornin, MIT License) and Boyar-Peralta S-box.
Adapted for facil.io FIO_MATH_UXXX vector types.

Two engines:
- 4-block engine: uint64_t q[8], processes 4 AES blocks per call.
  Used for H/J0 computation, CTR tail, and optionally all data.
- 16-block engine: fio_u256 q[8], processes 16 AES blocks per call.
  Used for large data (>=256 bytes) when FIO___AES_BS_WIDE=1.

Set FIO___AES_BS_WIDE=0 to use only the 4-block engine (for testing).
***************************************************************************** */

#ifndef FIO___AES_BS_WIDE
#define FIO___AES_BS_WIDE 1 /* 1 = use 16-block fio_u256 for large data */
#endif

/* ============================================================================
 * 1. Interleave: pack/unpack AES block between 4×uint32_t LE and 2×uint64_t
 * ========================================================================= */

/**
 * Pack one AES block (4 little-endian uint32_t words) into two uint64_t
 * values in the column-interleaved format used by the bitsliced pipeline.
 *
 * After interleave_in + ortho, data is in true bitsliced form.
 */
FIO_IFUNC void fio___aes_bs_interleave_in(uint64_t *q0,
                                          uint64_t *q1,
                                          const uint32_t *w) {
  uint64_t x0, x1, x2, x3;
  x0 = w[0];
  x1 = w[1];
  x2 = w[2];
  x3 = w[3];
  /* Spread each 32-bit word so bytes are in 16-bit groups */
  x0 |= (x0 << 16);
  x1 |= (x1 << 16);
  x2 |= (x2 << 16);
  x3 |= (x3 << 16);
  x0 &= (uint64_t)0x0000FFFF0000FFFF;
  x1 &= (uint64_t)0x0000FFFF0000FFFF;
  x2 &= (uint64_t)0x0000FFFF0000FFFF;
  x3 &= (uint64_t)0x0000FFFF0000FFFF;
  /* Further spread so bytes are in 8-bit groups */
  x0 |= (x0 << 8);
  x1 |= (x1 << 8);
  x2 |= (x2 << 8);
  x3 |= (x3 << 8);
  x0 &= (uint64_t)0x00FF00FF00FF00FF;
  x1 &= (uint64_t)0x00FF00FF00FF00FF;
  x2 &= (uint64_t)0x00FF00FF00FF00FF;
  x3 &= (uint64_t)0x00FF00FF00FF00FF;
  /* Combine: even columns in q0, odd columns in q1 */
  *q0 = x0 | (x2 << 8);
  *q1 = x1 | (x3 << 8);
}

/**
 * Unpack two uint64_t values back to 4 little-endian uint32_t words.
 * Inverse of fio___aes_bs_interleave_in.
 */
FIO_IFUNC void fio___aes_bs_interleave_out(uint32_t *w,
                                           uint64_t q0,
                                           uint64_t q1) {
  uint64_t x0, x1, x2, x3;
  x0 = q0 & (uint64_t)0x00FF00FF00FF00FF;
  x1 = q1 & (uint64_t)0x00FF00FF00FF00FF;
  x2 = (q0 >> 8) & (uint64_t)0x00FF00FF00FF00FF;
  x3 = (q1 >> 8) & (uint64_t)0x00FF00FF00FF00FF;
  x0 |= (x0 >> 8);
  x1 |= (x1 >> 8);
  x2 |= (x2 >> 8);
  x3 |= (x3 >> 8);
  x0 &= (uint64_t)0x0000FFFF0000FFFF;
  x1 &= (uint64_t)0x0000FFFF0000FFFF;
  x2 &= (uint64_t)0x0000FFFF0000FFFF;
  x3 &= (uint64_t)0x0000FFFF0000FFFF;
  w[0] = (uint32_t)x0 | (uint32_t)(x0 >> 16);
  w[1] = (uint32_t)x1 | (uint32_t)(x1 >> 16);
  w[2] = (uint32_t)x2 | (uint32_t)(x2 >> 16);
  w[3] = (uint32_t)x3 | (uint32_t)(x3 >> 16);
}

/* ============================================================================
 * 2. Ortho: bit transpose on uint64_t q[8] (self-inverse butterfly)
 * ========================================================================= */

/**
 * Transpose 8 uint64_t registers between byte-oriented and bitsliced form.
 * Self-inverse: apply before encryption to bitslice, after to un-bitslice.
 */
FIO_IFUNC void fio___aes_bs_ortho(uint64_t *q) {
#define FIO___AES_BS_SWAPN(cl, ch, s, x, y)                                    \
  do {                                                                         \
    uint64_t a_, b_;                                                           \
    a_ = (x);                                                                  \
    b_ = (y);                                                                  \
    (x) = (a_ & (uint64_t)(cl)) | ((b_ & (uint64_t)(cl)) << (s));              \
    (y) = ((a_ & (uint64_t)(ch)) >> (s)) | (b_ & (uint64_t)(ch));              \
  } while (0)

#define FIO___AES_BS_SWAP2(x, y)                                               \
  FIO___AES_BS_SWAPN(0x5555555555555555ULL, 0xAAAAAAAAAAAAAAAAULL, 1, x, y)
#define FIO___AES_BS_SWAP4(x, y)                                               \
  FIO___AES_BS_SWAPN(0x3333333333333333ULL, 0xCCCCCCCCCCCCCCCCULL, 2, x, y)
#define FIO___AES_BS_SWAP8(x, y)                                               \
  FIO___AES_BS_SWAPN(0x0F0F0F0F0F0F0F0FULL, 0xF0F0F0F0F0F0F0F0ULL, 4, x, y)

  FIO___AES_BS_SWAP2(q[0], q[1]);
  FIO___AES_BS_SWAP2(q[2], q[3]);
  FIO___AES_BS_SWAP2(q[4], q[5]);
  FIO___AES_BS_SWAP2(q[6], q[7]);

  FIO___AES_BS_SWAP4(q[0], q[2]);
  FIO___AES_BS_SWAP4(q[1], q[3]);
  FIO___AES_BS_SWAP4(q[4], q[6]);
  FIO___AES_BS_SWAP4(q[5], q[7]);

  FIO___AES_BS_SWAP8(q[0], q[4]);
  FIO___AES_BS_SWAP8(q[1], q[5]);
  FIO___AES_BS_SWAP8(q[2], q[6]);
  FIO___AES_BS_SWAP8(q[3], q[7]);

#undef FIO___AES_BS_SWAP8
#undef FIO___AES_BS_SWAP4
#undef FIO___AES_BS_SWAP2
#undef FIO___AES_BS_SWAPN
}

/* ============================================================================
 * 3. Boyar-Peralta S-box on fio_u256 q[8] (vectorized, 16 blocks)
 *
 * Input:  x0=q[7], x1=q[6], ..., x7=q[0]  (MSB first)
 * Output: q[7]=s0, q[6]=s1, ..., q[0]=s7
 *
 * 107 operations: 32 AND, 71 XOR, 4 XNOR (complement via XOR with all-ones)
 * ========================================================================= */

/* Helper macros for vectorized bitwise operations on fio_u256 */
#define FIO___AES_BS_XOR(dst, a, b)                                            \
  FIO_MATH_UXXX_OP((dst).x64, (a).x64, (b).x64, 64, ^)
#define FIO___AES_BS_AND(dst, a, b)                                            \
  FIO_MATH_UXXX_OP((dst).x64, (a).x64, (b).x64, 64, &)
/* XNOR: dst = a ^ b ^ all_ones = ~(a ^ b) */
#define FIO___AES_BS_XNOR(dst, a, b)                                           \
  do {                                                                         \
    FIO_MATH_UXXX_OP((dst).x64, (a).x64, (b).x64, 64, ^);                      \
    FIO_MATH_UXXX_COP((dst).x64, (dst).x64, ~(uint64_t)0, 64, ^);              \
  } while (0)

FIO_IFUNC void fio___aes_bs_sbox(fio_u256 *q) {
  fio_u256 x0, x1, x2, x3, x4, x5, x6, x7;
  fio_u256 y1, y2, y3, y4, y5, y6, y7, y8, y9;
  fio_u256 y10, y11, y12, y13, y14, y15, y16, y17, y18, y19;
  fio_u256 y20, y21;
  fio_u256 z0, z1, z2, z3, z4, z5, z6, z7, z8, z9;
  fio_u256 z10, z11, z12, z13, z14, z15, z16, z17;
  fio_u256 t0, t1, t2, t3, t4, t5, t6, t7, t8, t9;
  fio_u256 t10, t11, t12, t13, t14, t15, t16, t17, t18, t19;
  fio_u256 t20, t21, t22, t23, t24, t25, t26, t27, t28, t29;
  fio_u256 t30, t31, t32, t33, t34, t35, t36, t37, t38, t39;
  fio_u256 t40, t41, t42, t43, t44, t45, t46, t47, t48, t49;
  fio_u256 t50, t51, t52, t53, t54, t55, t56, t57, t58, t59;
  fio_u256 t60, t61, t62, t63, t64, t65, t66, t67;
  fio_u256 s0, s1, s2, s3, s4, s5, s6, s7;

  x0 = q[7];
  x1 = q[6];
  x2 = q[5];
  x3 = q[4];
  x4 = q[3];
  x5 = q[2];
  x6 = q[1];
  x7 = q[0];

  /* ===== TOP LINEAR TRANSFORMATION (23 XOR gates) ===== */
  FIO___AES_BS_XOR(y14, x3, x5);
  FIO___AES_BS_XOR(y13, x0, x6);
  FIO___AES_BS_XOR(y9, x0, x3);
  FIO___AES_BS_XOR(y8, x0, x5);
  FIO___AES_BS_XOR(t0, x1, x2);
  FIO___AES_BS_XOR(y1, t0, x7);
  FIO___AES_BS_XOR(y4, y1, x3);
  FIO___AES_BS_XOR(y12, y13, y14);
  FIO___AES_BS_XOR(y2, y1, x0);
  FIO___AES_BS_XOR(y5, y1, x6);
  FIO___AES_BS_XOR(y3, y5, y8);
  FIO___AES_BS_XOR(t1, x4, y12);
  FIO___AES_BS_XOR(y15, t1, x5);
  FIO___AES_BS_XOR(y20, t1, x1);
  FIO___AES_BS_XOR(y6, y15, x7);
  FIO___AES_BS_XOR(y10, y15, t0);
  FIO___AES_BS_XOR(y11, y20, y9);
  FIO___AES_BS_XOR(y7, x7, y11);
  FIO___AES_BS_XOR(y17, y10, y11);
  FIO___AES_BS_XOR(y19, y10, y8);
  FIO___AES_BS_XOR(y16, t0, y11);
  FIO___AES_BS_XOR(y21, y13, y16);
  FIO___AES_BS_XOR(y18, x0, y16);

  /* ===== NON-LINEAR SECTION (32 AND + 22 XOR = 54 gates) ===== */
  FIO___AES_BS_AND(t2, y12, y15);
  FIO___AES_BS_AND(t3, y3, y6);
  FIO___AES_BS_XOR(t4, t3, t2);
  FIO___AES_BS_AND(t5, y4, x7);
  FIO___AES_BS_XOR(t6, t5, t2);
  FIO___AES_BS_AND(t7, y13, y16);
  FIO___AES_BS_AND(t8, y5, y1);
  FIO___AES_BS_XOR(t9, t8, t7);
  FIO___AES_BS_AND(t10, y2, y7);
  FIO___AES_BS_XOR(t11, t10, t7);
  FIO___AES_BS_AND(t12, y9, y11);
  FIO___AES_BS_AND(t13, y14, y17);
  FIO___AES_BS_XOR(t14, t13, t12);
  FIO___AES_BS_AND(t15, y8, y10);
  FIO___AES_BS_XOR(t16, t15, t12);
  FIO___AES_BS_XOR(t17, t4, y20);
  FIO___AES_BS_XOR(t18, t6, t16);
  FIO___AES_BS_XOR(t19, t9, t14);
  FIO___AES_BS_XOR(t20, t11, t16);
  FIO___AES_BS_XOR(t21, t17, t14);
  FIO___AES_BS_XOR(t22, t18, y19);
  FIO___AES_BS_XOR(t23, t19, y21);
  FIO___AES_BS_XOR(t24, t20, y18);

  /* GF(2^4) inversion core */
  FIO___AES_BS_XOR(t25, t21, t22);
  FIO___AES_BS_AND(t26, t21, t23);
  FIO___AES_BS_XOR(t27, t24, t26);
  FIO___AES_BS_AND(t28, t25, t27);
  FIO___AES_BS_XOR(t29, t28, t22);
  FIO___AES_BS_XOR(t30, t23, t24);
  FIO___AES_BS_XOR(t31, t22, t26);
  FIO___AES_BS_AND(t32, t31, t30);
  FIO___AES_BS_XOR(t33, t32, t24);
  FIO___AES_BS_XOR(t34, t23, t33);
  FIO___AES_BS_XOR(t35, t27, t33);
  FIO___AES_BS_AND(t36, t24, t35);
  FIO___AES_BS_XOR(t37, t36, t34);
  FIO___AES_BS_XOR(t38, t27, t36);
  FIO___AES_BS_AND(t39, t29, t38);
  FIO___AES_BS_XOR(t40, t25, t39);

  /* Combine inversion results */
  FIO___AES_BS_XOR(t41, t40, t37);
  FIO___AES_BS_XOR(t42, t29, t33);
  FIO___AES_BS_XOR(t43, t29, t40);
  FIO___AES_BS_XOR(t44, t33, t37);
  FIO___AES_BS_XOR(t45, t42, t41);

  /* Multiply by input shares (18 AND gates) */
  FIO___AES_BS_AND(z0, t44, y15);
  FIO___AES_BS_AND(z1, t37, y6);
  FIO___AES_BS_AND(z2, t33, x7);
  FIO___AES_BS_AND(z3, t43, y16);
  FIO___AES_BS_AND(z4, t40, y1);
  FIO___AES_BS_AND(z5, t29, y7);
  FIO___AES_BS_AND(z6, t42, y11);
  FIO___AES_BS_AND(z7, t45, y17);
  FIO___AES_BS_AND(z8, t41, y10);
  FIO___AES_BS_AND(z9, t44, y12);
  FIO___AES_BS_AND(z10, t37, y3);
  FIO___AES_BS_AND(z11, t33, y4);
  FIO___AES_BS_AND(z12, t43, y13);
  FIO___AES_BS_AND(z13, t40, y5);
  FIO___AES_BS_AND(z14, t29, y2);
  FIO___AES_BS_AND(z15, t42, y9);
  FIO___AES_BS_AND(z16, t45, y14);
  FIO___AES_BS_AND(z17, t41, y8);

  /* ===== BOTTOM LINEAR TRANSFORMATION (26 XOR + 4 XNOR = 30 gates) ===== */
  FIO___AES_BS_XOR(t46, z15, z16);
  FIO___AES_BS_XOR(t47, z10, z11);
  FIO___AES_BS_XOR(t48, z5, z13);
  FIO___AES_BS_XOR(t49, z9, z10);
  FIO___AES_BS_XOR(t50, z2, z12);
  FIO___AES_BS_XOR(t51, z2, z5);
  FIO___AES_BS_XOR(t52, z7, z8);
  FIO___AES_BS_XOR(t53, z0, z3);
  FIO___AES_BS_XOR(t54, z6, z7);
  FIO___AES_BS_XOR(t55, z16, z17);
  FIO___AES_BS_XOR(t56, z12, t48);
  FIO___AES_BS_XOR(t57, t50, t53);
  FIO___AES_BS_XOR(t58, z4, t46);
  FIO___AES_BS_XOR(t59, z3, t54);
  FIO___AES_BS_XOR(t60, t46, t57);
  FIO___AES_BS_XOR(t61, z14, t57);
  FIO___AES_BS_XOR(t62, t52, t58);
  FIO___AES_BS_XOR(t63, t49, t58);
  FIO___AES_BS_XOR(t64, z4, t59);
  FIO___AES_BS_XOR(t65, t61, t62);
  FIO___AES_BS_XOR(t66, z1, t63);
  FIO___AES_BS_XOR(s0, t59, t63);
  FIO___AES_BS_XNOR(s6, t56, t62);
  FIO___AES_BS_XNOR(s7, t48, t60);
  FIO___AES_BS_XOR(t67, t64, t65);
  FIO___AES_BS_XOR(s3, t53, t66);
  FIO___AES_BS_XOR(s4, t51, t66);
  FIO___AES_BS_XOR(s5, t47, t65);
  FIO___AES_BS_XNOR(s1, t64, s3);
  FIO___AES_BS_XNOR(s2, t55, t67);

  q[7] = s0;
  q[6] = s1;
  q[5] = s2;
  q[4] = s3;
  q[3] = s4;
  q[2] = s5;
  q[1] = s6;
  q[0] = s7;
}

#undef FIO___AES_BS_XOR
#undef FIO___AES_BS_AND
#undef FIO___AES_BS_XNOR

/* ============================================================================
 * 4. ShiftRows on fio_u256 q[8]
 *
 * Each bit plane is processed identically. Within each uint64_t lane:
 *   Row 0 (bits 0-3 per 16-bit block): no shift
 *   Row 1 (bits 4-7): rotate left by 1 column (4-bit nibble shift)
 *   Row 2 (bits 8-11): rotate left by 2 columns (8-bit shift)
 *   Row 3 (bits 12-15): rotate left by 3 columns (12-bit shift)
 * ========================================================================= */

FIO_IFUNC void fio___aes_bs_shift_rows(fio_u256 *q) {
  for (int i = 0; i < 8; ++i) {
    for (size_t lane = 0; lane < 4; ++lane) {
      uint64_t x = q[i].u64[lane];
      q[i].u64[lane] = (x & (uint64_t)0x000000000000FFFFULL) |
                       ((x & (uint64_t)0x00000000FFF00000ULL) >> 4) |
                       ((x & (uint64_t)0x00000000000F0000ULL) << 12) |
                       ((x & (uint64_t)0x0000FF0000000000ULL) >> 8) |
                       ((x & (uint64_t)0x000000FF00000000ULL) << 8) |
                       ((x & (uint64_t)0xF000000000000000ULL) >> 12) |
                       ((x & (uint64_t)0x0FFF000000000000ULL) << 4);
    }
  }
}

/* ============================================================================
 * 5. MixColumns on fio_u256 q[8]
 *
 * Per uint64_t lane: 16-bit left rotation for r[i], 32-bit half-swap for
 * rotr32. XOR formula implements the AES MixColumns polynomial
 * {03}x^3 + {01}x^2 + {01}x + {02} with xtime reduction via q7 feedback
 * to bits 0, 1, 3, 4 (polynomial x^8 + x^4 + x^3 + x + 1).
 * ========================================================================= */

FIO_IFUNC void fio___aes_bs_mix_columns(fio_u256 *q) {
  fio_u256 q0, q1, q2, q3, q4, q5, q6, q7;
  fio_u256 r0, r1, r2, r3, r4, r5, r6, r7;

  q0 = q[0];
  q1 = q[1];
  q2 = q[2];
  q3 = q[3];
  q4 = q[4];
  q5 = q[5];
  q6 = q[6];
  q7 = q[7];

  /* r[i] = rotate q[i] left by 16 bits (per uint64_t lane) */
  for (size_t lane = 0; lane < 4; ++lane) {
    r0.u64[lane] = (q0.u64[lane] >> 16) | (q0.u64[lane] << 48);
    r1.u64[lane] = (q1.u64[lane] >> 16) | (q1.u64[lane] << 48);
    r2.u64[lane] = (q2.u64[lane] >> 16) | (q2.u64[lane] << 48);
    r3.u64[lane] = (q3.u64[lane] >> 16) | (q3.u64[lane] << 48);
    r4.u64[lane] = (q4.u64[lane] >> 16) | (q4.u64[lane] << 48);
    r5.u64[lane] = (q5.u64[lane] >> 16) | (q5.u64[lane] << 48);
    r6.u64[lane] = (q6.u64[lane] >> 16) | (q6.u64[lane] << 48);
    r7.u64[lane] = (q7.u64[lane] >> 16) | (q7.u64[lane] << 48);
  }

  /*
   * q'[i] = q[i-1] ^ r[i-1] ^ r[i] ^ rotr32(q[i] ^ r[i])
   * where rotr32 swaps the two 32-bit halves of each uint64_t lane.
   * Bits 1, 3, 4 also XOR in q7^r7 (AES irreducible polynomial feedback).
   */
  for (size_t lane = 0; lane < 4; ++lane) {
    uint64_t qr0 = q0.u64[lane] ^ r0.u64[lane];
    uint64_t qr1 = q1.u64[lane] ^ r1.u64[lane];
    uint64_t qr2 = q2.u64[lane] ^ r2.u64[lane];
    uint64_t qr3 = q3.u64[lane] ^ r3.u64[lane];
    uint64_t qr4 = q4.u64[lane] ^ r4.u64[lane];
    uint64_t qr5 = q5.u64[lane] ^ r5.u64[lane];
    uint64_t qr6 = q6.u64[lane] ^ r6.u64[lane];
    uint64_t qr7 = q7.u64[lane] ^ r7.u64[lane];
    uint64_t q7r7 = qr7; /* q7 ^ r7 for polynomial reduction */

    q[0].u64[lane] = q7.u64[lane] ^ r7.u64[lane] ^ r0.u64[lane] ^
                     ((qr0 << 32) | (qr0 >> 32));
    q[1].u64[lane] = q0.u64[lane] ^ r0.u64[lane] ^ q7r7 ^ r1.u64[lane] ^
                     ((qr1 << 32) | (qr1 >> 32));
    q[2].u64[lane] = q1.u64[lane] ^ r1.u64[lane] ^ r2.u64[lane] ^
                     ((qr2 << 32) | (qr2 >> 32));
    q[3].u64[lane] = q2.u64[lane] ^ r2.u64[lane] ^ q7r7 ^ r3.u64[lane] ^
                     ((qr3 << 32) | (qr3 >> 32));
    q[4].u64[lane] = q3.u64[lane] ^ r3.u64[lane] ^ q7r7 ^ r4.u64[lane] ^
                     ((qr4 << 32) | (qr4 >> 32));
    q[5].u64[lane] = q4.u64[lane] ^ r4.u64[lane] ^ r5.u64[lane] ^
                     ((qr5 << 32) | (qr5 >> 32));
    q[6].u64[lane] = q5.u64[lane] ^ r5.u64[lane] ^ r6.u64[lane] ^
                     ((qr6 << 32) | (qr6 >> 32));
    q[7].u64[lane] = q6.u64[lane] ^ r6.u64[lane] ^ r7.u64[lane] ^
                     ((qr7 << 32) | (qr7 >> 32));
  }
}

/* ============================================================================
 * 6. AddRoundKey: XOR 8 bit planes with 8 round key vectors
 * ========================================================================= */

/**
 * XOR round key into state. The round key sk[0..7] must already be in
 * expanded form (one uint64_t per bit plane, broadcast across all lanes).
 */
FIO_IFUNC void fio___aes_bs_add_round_key(fio_u256 *q, const uint64_t *sk) {
  for (int i = 0; i < 8; ++i) {
    FIO_MATH_UXXX_COP(q[i].x64, q[i].x64, sk[i], 64, ^);
  }
}

/* ============================================================================
 * 7. sub_word: pack single uint32_t through S-box for key expansion
 *
 * Uses scalar uint64_t q[8] with ortho + sbox_scalar + ortho.
 * ========================================================================= */

/** Scalar S-box on uint64_t q[8] (processes 4 blocks packed in uint64_t). */
FIO_SFUNC void fio___aes_bs_sbox_scalar(uint64_t *q) {
  uint64_t x0, x1, x2, x3, x4, x5, x6, x7;
  uint64_t y1, y2, y3, y4, y5, y6, y7, y8, y9;
  uint64_t y10, y11, y12, y13, y14, y15, y16, y17, y18, y19;
  uint64_t y20, y21;
  uint64_t z0, z1, z2, z3, z4, z5, z6, z7, z8, z9;
  uint64_t z10, z11, z12, z13, z14, z15, z16, z17;
  uint64_t t0, t1, t2, t3, t4, t5, t6, t7, t8, t9;
  uint64_t t10, t11, t12, t13, t14, t15, t16, t17, t18, t19;
  uint64_t t20, t21, t22, t23, t24, t25, t26, t27, t28, t29;
  uint64_t t30, t31, t32, t33, t34, t35, t36, t37, t38, t39;
  uint64_t t40, t41, t42, t43, t44, t45, t46, t47, t48, t49;
  uint64_t t50, t51, t52, t53, t54, t55, t56, t57, t58, t59;
  uint64_t t60, t61, t62, t63, t64, t65, t66, t67;
  uint64_t s0, s1, s2, s3, s4, s5, s6, s7;

  x0 = q[7];
  x1 = q[6];
  x2 = q[5];
  x3 = q[4];
  x4 = q[3];
  x5 = q[2];
  x6 = q[1];
  x7 = q[0];

  /* Top linear */
  y14 = x3 ^ x5;
  y13 = x0 ^ x6;
  y9 = x0 ^ x3;
  y8 = x0 ^ x5;
  t0 = x1 ^ x2;
  y1 = t0 ^ x7;
  y4 = y1 ^ x3;
  y12 = y13 ^ y14;
  y2 = y1 ^ x0;
  y5 = y1 ^ x6;
  y3 = y5 ^ y8;
  t1 = x4 ^ y12;
  y15 = t1 ^ x5;
  y20 = t1 ^ x1;
  y6 = y15 ^ x7;
  y10 = y15 ^ t0;
  y11 = y20 ^ y9;
  y7 = x7 ^ y11;
  y17 = y10 ^ y11;
  y19 = y10 ^ y8;
  y16 = t0 ^ y11;
  y21 = y13 ^ y16;
  y18 = x0 ^ y16;

  /* Non-linear */
  t2 = y12 & y15;
  t3 = y3 & y6;
  t4 = t3 ^ t2;
  t5 = y4 & x7;
  t6 = t5 ^ t2;
  t7 = y13 & y16;
  t8 = y5 & y1;
  t9 = t8 ^ t7;
  t10 = y2 & y7;
  t11 = t10 ^ t7;
  t12 = y9 & y11;
  t13 = y14 & y17;
  t14 = t13 ^ t12;
  t15 = y8 & y10;
  t16 = t15 ^ t12;
  t17 = t4 ^ y20;
  t18 = t6 ^ t16;
  t19 = t9 ^ t14;
  t20 = t11 ^ t16;
  t21 = t17 ^ t14;
  t22 = t18 ^ y19;
  t23 = t19 ^ y21;
  t24 = t20 ^ y18;

  t25 = t21 ^ t22;
  t26 = t21 & t23;
  t27 = t24 ^ t26;
  t28 = t25 & t27;
  t29 = t28 ^ t22;
  t30 = t23 ^ t24;
  t31 = t22 ^ t26;
  t32 = t31 & t30;
  t33 = t32 ^ t24;
  t34 = t23 ^ t33;
  t35 = t27 ^ t33;
  t36 = t24 & t35;
  t37 = t36 ^ t34;
  t38 = t27 ^ t36;
  t39 = t29 & t38;
  t40 = t25 ^ t39;

  t41 = t40 ^ t37;
  t42 = t29 ^ t33;
  t43 = t29 ^ t40;
  t44 = t33 ^ t37;
  t45 = t42 ^ t41;

  z0 = t44 & y15;
  z1 = t37 & y6;
  z2 = t33 & x7;
  z3 = t43 & y16;
  z4 = t40 & y1;
  z5 = t29 & y7;
  z6 = t42 & y11;
  z7 = t45 & y17;
  z8 = t41 & y10;
  z9 = t44 & y12;
  z10 = t37 & y3;
  z11 = t33 & y4;
  z12 = t43 & y13;
  z13 = t40 & y5;
  z14 = t29 & y2;
  z15 = t42 & y9;
  z16 = t45 & y14;
  z17 = t41 & y8;

  /* Bottom linear */
  t46 = z15 ^ z16;
  t47 = z10 ^ z11;
  t48 = z5 ^ z13;
  t49 = z9 ^ z10;
  t50 = z2 ^ z12;
  t51 = z2 ^ z5;
  t52 = z7 ^ z8;
  t53 = z0 ^ z3;
  t54 = z6 ^ z7;
  t55 = z16 ^ z17;
  t56 = z12 ^ t48;
  t57 = t50 ^ t53;
  t58 = z4 ^ t46;
  t59 = z3 ^ t54;
  t60 = t46 ^ t57;
  t61 = z14 ^ t57;
  t62 = t52 ^ t58;
  t63 = t49 ^ t58;
  t64 = z4 ^ t59;
  t65 = t61 ^ t62;
  t66 = z1 ^ t63;
  s0 = t59 ^ t63;
  s6 = t56 ^ t62;
  s6 = ~s6;
  s7 = t48 ^ t60;
  s7 = ~s7;
  t67 = t64 ^ t65;
  s3 = t53 ^ t66;
  s4 = t51 ^ t66;
  s5 = t47 ^ t65;
  s1 = t64 ^ s3;
  s1 = ~s1;
  s2 = t55 ^ t67;
  s2 = ~s2;

  q[7] = s0;
  q[6] = s1;
  q[5] = s2;
  q[4] = s3;
  q[3] = s4;
  q[2] = s5;
  q[1] = s6;
  q[0] = s7;
}

/**
 * Apply S-box to a single 32-bit word (for key expansion).
 * Packs the word into scalar bitsliced form, applies S-box, unpacks.
 */
FIO_SFUNC uint32_t fio___aes_bs_sub_word(uint32_t x) {
  uint64_t q[8];
  FIO_MEMSET(q, 0, sizeof(q));
  q[0] = x;
  fio___aes_bs_ortho(q);
  fio___aes_bs_sbox_scalar(q);
  fio___aes_bs_ortho(q);
  return (uint32_t)q[0];
}

/**
 * Apply S-box to 4 words simultaneously using the 4-block engine.
 * Each word occupies one block slot. Returns 4 substituted words.
 * This amortizes the ortho + sbox cost across 4 words.
 */
FIO_SFUNC void fio___aes_bs_sub_word4(uint32_t out[4], const uint32_t in[4]) {
  uint64_t q[8];
  FIO_MEMSET(q, 0, sizeof(q));
  /* Pack each word into its own block slot via interleave_in */
  fio___aes_bs_interleave_in(&q[0], &q[4], in);
  /* Broadcast to all 4 slots (all words get same treatment) */
  /* Actually, interleave_in packs 4 uint32_t as one AES block.
   * For sub_word we need each uint32_t as a separate "block".
   * Use the same approach as sub_word but pack 4 values. */

  /* Reset and pack properly: each word in its own slot */
  FIO_MEMSET(q, 0, sizeof(q));
  q[0] = (uint64_t)in[0];
  q[1] = (uint64_t)in[1];
  q[2] = (uint64_t)in[2];
  q[3] = (uint64_t)in[3];
  /* q[4..7] = 0 */

  /* Ortho transposes bit planes across all 8 registers.
   * With data only in q[0..3], the SWAP8 step moves nibble-level
   * data between q[0..3] and q[4..7]. This is correct. */
  fio___aes_bs_ortho(q);
  fio___aes_bs_sbox_scalar(q);
  fio___aes_bs_ortho(q);

  out[0] = (uint32_t)q[0];
  out[1] = (uint32_t)q[1];
  out[2] = (uint32_t)q[2];
  out[3] = (uint32_t)q[3];
}

/* ============================================================================
 * 7b. 4-block scalar engine: ShiftRows, MixColumns, AddRoundKey, Encrypt
 *
 * Operates on uint64_t q[8] directly (4 AES blocks per uint64_t).
 * Much lighter than the 16-block fio_u256 engine for small workloads.
 * ========================================================================= */

/** ShiftRows on uint64_t q[8] (4 blocks). */
FIO_IFUNC void fio___aes_bs_shift_rows4(uint64_t *q) {
  for (int i = 0; i < 8; ++i) {
    uint64_t x = q[i];
    q[i] = (x & (uint64_t)0x000000000000FFFFULL) |
           ((x & (uint64_t)0x00000000FFF00000ULL) >> 4) |
           ((x & (uint64_t)0x00000000000F0000ULL) << 12) |
           ((x & (uint64_t)0x0000FF0000000000ULL) >> 8) |
           ((x & (uint64_t)0x000000FF00000000ULL) << 8) |
           ((x & (uint64_t)0xF000000000000000ULL) >> 12) |
           ((x & (uint64_t)0x0FFF000000000000ULL) << 4);
  }
}

/** MixColumns on uint64_t q[8] (4 blocks). */
FIO_IFUNC void fio___aes_bs_mix_columns4(uint64_t *q) {
  uint64_t q0, q1, q2, q3, q4, q5, q6, q7;
  uint64_t r0, r1, r2, r3, r4, r5, r6, r7;

  q0 = q[0];
  q1 = q[1];
  q2 = q[2];
  q3 = q[3];
  q4 = q[4];
  q5 = q[5];
  q6 = q[6];
  q7 = q[7];

  /* r[i] = rotate q[i] left by 16 bits */
  r0 = (q0 >> 16) | (q0 << 48);
  r1 = (q1 >> 16) | (q1 << 48);
  r2 = (q2 >> 16) | (q2 << 48);
  r3 = (q3 >> 16) | (q3 << 48);
  r4 = (q4 >> 16) | (q4 << 48);
  r5 = (q5 >> 16) | (q5 << 48);
  r6 = (q6 >> 16) | (q6 << 48);
  r7 = (q7 >> 16) | (q7 << 48);

  /* MixColumns formula with rotr32 = 32-bit half swap */
  {
    uint64_t qr0 = q0 ^ r0, qr1 = q1 ^ r1, qr2 = q2 ^ r2, qr3 = q3 ^ r3;
    uint64_t qr4 = q4 ^ r4, qr5 = q5 ^ r5, qr6 = q6 ^ r6, qr7 = q7 ^ r7;
    uint64_t q7r7 = qr7;

    q[0] = q7 ^ r7 ^ r0 ^ ((qr0 << 32) | (qr0 >> 32));
    q[1] = q0 ^ r0 ^ q7r7 ^ r1 ^ ((qr1 << 32) | (qr1 >> 32));
    q[2] = q1 ^ r1 ^ r2 ^ ((qr2 << 32) | (qr2 >> 32));
    q[3] = q2 ^ r2 ^ q7r7 ^ r3 ^ ((qr3 << 32) | (qr3 >> 32));
    q[4] = q3 ^ r3 ^ q7r7 ^ r4 ^ ((qr4 << 32) | (qr4 >> 32));
    q[5] = q4 ^ r4 ^ r5 ^ ((qr5 << 32) | (qr5 >> 32));
    q[6] = q5 ^ r5 ^ r6 ^ ((qr6 << 32) | (qr6 >> 32));
    q[7] = q6 ^ r6 ^ r7 ^ ((qr7 << 32) | (qr7 >> 32));
  }
}

/** AddRoundKey on uint64_t q[8] (4 blocks). sk[0..7] are expanded keys. */
FIO_IFUNC void fio___aes_bs_add_round_key4(uint64_t *q, const uint64_t *sk) {
  q[0] ^= sk[0];
  q[1] ^= sk[1];
  q[2] ^= sk[2];
  q[3] ^= sk[3];
  q[4] ^= sk[4];
  q[5] ^= sk[5];
  q[6] ^= sk[6];
  q[7] ^= sk[7];
}

/**
 * Expand one compressed key pair to 8 uint64_t round key values.
 * The (x<<4)-x trick broadcasts each bit to fill its 4-bit nibble.
 */
FIO_IFUNC void fio___aes_bs_expand_key_pair(uint64_t sk_out[8],
                                            uint64_t ck0,
                                            uint64_t ck1) {
  uint64_t x0, x1, x2, x3;
  x0 = x1 = x2 = x3 = ck0;
  x0 &= 0x1111111111111111ULL;
  x1 &= 0x2222222222222222ULL;
  x2 &= 0x4444444444444444ULL;
  x3 &= 0x8888888888888888ULL;
  x1 >>= 1;
  x2 >>= 2;
  x3 >>= 3;
  sk_out[0] = (x0 << 4) - x0;
  sk_out[1] = (x1 << 4) - x1;
  sk_out[2] = (x2 << 4) - x2;
  sk_out[3] = (x3 << 4) - x3;

  x0 = x1 = x2 = x3 = ck1;
  x0 &= 0x1111111111111111ULL;
  x1 &= 0x2222222222222222ULL;
  x2 &= 0x4444444444444444ULL;
  x3 &= 0x8888888888888888ULL;
  x1 >>= 1;
  x2 >>= 2;
  x3 >>= 3;
  sk_out[4] = (x0 << 4) - x0;
  sk_out[5] = (x1 << 4) - x1;
  sk_out[6] = (x2 << 4) - x2;
  sk_out[7] = (x3 << 4) - x3;
}

/**
 * Pre-expand ALL round keys for the 4-block engine.
 * Output: sk_exp4[(num_rounds+1)*8] uint64_t values.
 */
FIO_SFUNC void fio___aes_bs_expand_all_keys4(uint64_t *sk_exp4,
                                             unsigned num_rounds,
                                             const uint64_t *comp_skey) {
  for (unsigned u = 0; u <= num_rounds; ++u)
    fio___aes_bs_expand_key_pair(sk_exp4 + u * 8,
                                 comp_skey[u << 1],
                                 comp_skey[(u << 1) + 1]);
}

/**
 * Encrypt 4 blocks using the scalar 4-block engine.
 * sk_exp4: pre-expanded keys, (num_rounds+1)*8 uint64_t values.
 */
FIO_SFUNC void fio___aes_bs_encrypt4(unsigned num_rounds,
                                     const uint64_t *sk_exp4,
                                     uint64_t q[8]) {
  unsigned u;
  fio___aes_bs_add_round_key4(q, sk_exp4);
  for (u = 1; u < num_rounds; ++u) {
    fio___aes_bs_sbox_scalar(q);
    fio___aes_bs_shift_rows4(q);
    fio___aes_bs_mix_columns4(q);
    fio___aes_bs_add_round_key4(q, sk_exp4 + (u << 3));
  }
  fio___aes_bs_sbox_scalar(q);
  fio___aes_bs_shift_rows4(q);
  fio___aes_bs_add_round_key4(q, sk_exp4 + (num_rounds << 3));
}

/**
 * Encrypt 4 blocks using compressed keys (on-the-fly expansion).
 * Avoids the need for a pre-expanded key array, saving ~700 bytes of
 * stack space and the corresponding secure_zero cost.
 * comp_skey: compressed round keys, 2 uint64_t per round.
 */
FIO_SFUNC void fio___aes_bs_encrypt4_comp(unsigned num_rounds,
                                          const uint64_t *comp_skey,
                                          uint64_t q[8]) {
  uint64_t sk[8];
  unsigned u;
  fio___aes_bs_expand_key_pair(sk, comp_skey[0], comp_skey[1]);
  fio___aes_bs_add_round_key4(q, sk);
  for (u = 1; u < num_rounds; ++u) {
    fio___aes_bs_sbox_scalar(q);
    fio___aes_bs_shift_rows4(q);
    fio___aes_bs_mix_columns4(q);
    fio___aes_bs_expand_key_pair(sk,
                                 comp_skey[u << 1],
                                 comp_skey[(u << 1) + 1]);
    fio___aes_bs_add_round_key4(q, sk);
  }
  fio___aes_bs_sbox_scalar(q);
  fio___aes_bs_shift_rows4(q);
  fio___aes_bs_expand_key_pair(sk,
                               comp_skey[num_rounds << 1],
                               comp_skey[(num_rounds << 1) + 1]);
  fio___aes_bs_add_round_key4(q, sk);
  fio_secure_zero(sk, sizeof(sk));
}

/**
 * Encrypt a single AES block using the 4-block engine.
 * Much faster than fio___aes_bs_encrypt_block which uses the 16-block engine.
 */
FIO_SFUNC void fio___aes_bs_encrypt_block4(uint32_t out[4],
                                           const uint32_t in[4],
                                           unsigned num_rounds,
                                           const uint64_t *sk_exp4) {
  uint64_t q[8];
  FIO_MEMSET(q, 0, sizeof(q));
  fio___aes_bs_interleave_in(&q[0], &q[4], in);
  q[1] = q[0];
  q[2] = q[0];
  q[3] = q[0];
  q[5] = q[4];
  q[6] = q[4];
  q[7] = q[4];
  fio___aes_bs_ortho(q);
  fio___aes_bs_encrypt4(num_rounds, sk_exp4, q);
  fio___aes_bs_ortho(q);
  fio___aes_bs_interleave_out(out, q[0], q[4]);
}

/**
 * Encrypt two blocks in one 4-block call (H + J0 batched).
 * Block 0 = in0 -> q[0]/q[4], Block 1 = in1 -> q[1]/q[5].
 * Remaining slots duplicate block 0 (don't care, just need valid data).
 */
FIO_SFUNC void fio___aes_bs_encrypt_2blocks(uint32_t out0[4],
                                            uint32_t out1[4],
                                            const uint32_t in0[4],
                                            const uint32_t in1[4],
                                            unsigned num_rounds,
                                            const uint64_t *sk_exp4) {
  uint64_t q[8];
  /* Block 0 -> slot 0, Block 1 -> slot 1, slots 2-3 = copies of block 0 */
  fio___aes_bs_interleave_in(&q[0], &q[4], in0);
  fio___aes_bs_interleave_in(&q[1], &q[5], in1);
  q[2] = q[0];
  q[3] = q[0];
  q[6] = q[4];
  q[7] = q[4];

  fio___aes_bs_ortho(q);
  fio___aes_bs_encrypt4(num_rounds, sk_exp4, q);
  fio___aes_bs_ortho(q);

  /* Extract block 0 from slot 0, block 1 from slot 1 */
  fio___aes_bs_interleave_out(out0, q[0], q[4]);
  fio___aes_bs_interleave_out(out1, q[1], q[5]);
}

/**
 * Encrypt 4 counter blocks and produce 64 bytes of keystream.
 * Uses the 4-block scalar engine.
 */
FIO_SFUNC void fio___aes_bs_ctr_encrypt4_ks(uint8_t ks[64],
                                            const uint32_t w[16],
                                            unsigned num_rounds,
                                            const uint64_t *sk_exp4) {
  uint64_t q[8];

  /* Pack 4 blocks: block b -> interleave_in(&q[b], &q[b+4]) */
  for (unsigned b = 0; b < 4; ++b)
    fio___aes_bs_interleave_in(&q[b], &q[b + 4], w + b * 4);

  fio___aes_bs_ortho(q);
  fio___aes_bs_encrypt4(num_rounds, sk_exp4, q);
  fio___aes_bs_ortho(q);

  /* Unpack 4 blocks */
  for (unsigned b = 0; b < 4; ++b) {
    uint32_t out_w[4];
    fio___aes_bs_interleave_out(out_w, q[b], q[b + 4]);
    fio_u2buf32_le(ks + b * 16, out_w[0]);
    fio_u2buf32_le(ks + b * 16 + 4, out_w[1]);
    fio_u2buf32_le(ks + b * 16 + 8, out_w[2]);
    fio_u2buf32_le(ks + b * 16 + 12, out_w[3]);
  }
}

/** Prepare 4 counter blocks as 16 uint32_t words. */
FIO_SFUNC void fio___aes_bs_ctr_prepare4(uint32_t w[16],
                                         const uint32_t nonce_w[3],
                                         uint32_t ctr_start) {
  for (unsigned b = 0; b < 4; ++b) {
    w[b * 4 + 0] = nonce_w[0];
    w[b * 4 + 1] = nonce_w[1];
    w[b * 4 + 2] = nonce_w[2];
    w[b * 4 + 3] = fio_lton32(ctr_start + b);
  }
}

/* ============================================================================
 * 8. Key Expansion: AES-128 and AES-256
 *
 * Phase 1: Standard AES key schedule using sub_word (bitsliced S-box)
 * Phase 2: Convert expanded key to compressed bitsliced format
 *          (2 uint64_t per round key)
 * ========================================================================= */

/* AES round constants for key expansion (Rcon[1..10], little-endian byte 0) */
static const uint32_t FIO___AES_BS_RCON[] =
    {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1B, 0x36};

/**
 * AES-128 key expansion to compressed bitsliced round keys.
 * Output: comp_skey[22] (2 uint64_t per round × 11 rounds)
 */
FIO_SFUNC void fio___aes_bs_key_expand128(uint64_t comp_skey[22],
                                          const uint8_t key[16]) {
  uint32_t skey[44];
  int i;

  /* Phase 1: standard AES-128 key schedule (little-endian words) */
  skey[0] = fio_buf2u32_le(key);
  skey[1] = fio_buf2u32_le(key + 4);
  skey[2] = fio_buf2u32_le(key + 8);
  skey[3] = fio_buf2u32_le(key + 12);

  for (i = 4; i < 44; ++i) {
    uint32_t tmp = skey[i - 1];
    if ((i & 3) == 0) {
      /* RotWord: rotate left by 1 byte (LE uint32_t: shift right 8) */
      tmp = (tmp << 24) | (tmp >> 8);
      /* SubWord: apply S-box via bitsliced circuit */
      tmp = fio___aes_bs_sub_word(tmp);
      /* XOR with Rcon (only affects lowest byte in LE representation) */
      tmp ^= FIO___AES_BS_RCON[(i >> 2) - 1];
    }
    skey[i] = skey[i - 4] ^ tmp;
  }

  /* Phase 2: compress into bitsliced format */
  for (i = 0; i < 44; i += 4) {
    uint64_t q[8];
    int j = (i >> 1); /* i/4 * 2 = i/2 */
    fio___aes_bs_interleave_in(&q[0], &q[4], skey + i);
    q[1] = q[0];
    q[2] = q[0];
    q[3] = q[0];
    q[5] = q[4];
    q[6] = q[4];
    q[7] = q[4];
    fio___aes_bs_ortho(q);
    comp_skey[j + 0] = (q[0] & (uint64_t)0x1111111111111111ULL) |
                       (q[1] & (uint64_t)0x2222222222222222ULL) |
                       (q[2] & (uint64_t)0x4444444444444444ULL) |
                       (q[3] & (uint64_t)0x8888888888888888ULL);
    comp_skey[j + 1] = (q[4] & (uint64_t)0x1111111111111111ULL) |
                       (q[5] & (uint64_t)0x2222222222222222ULL) |
                       (q[6] & (uint64_t)0x4444444444444444ULL) |
                       (q[7] & (uint64_t)0x8888888888888888ULL);
  }
}

/**
 * AES-256 key expansion to compressed bitsliced round keys.
 * Output: comp_skey[30] (2 uint64_t per round × 15 rounds)
 */
FIO_SFUNC void fio___aes_bs_key_expand256(uint64_t comp_skey[30],
                                          const uint8_t key[32]) {
  uint32_t skey[60];
  int i;

  /* Phase 1: standard AES-256 key schedule (little-endian words) */
  for (i = 0; i < 8; ++i)
    skey[i] = fio_buf2u32_le(key + 4 * i);

  for (i = 8; i < 60; ++i) {
    uint32_t tmp = skey[i - 1];
    if ((i & 7) == 0) {
      /* RotWord + SubWord + Rcon */
      tmp = (tmp << 24) | (tmp >> 8);
      tmp = fio___aes_bs_sub_word(tmp);
      tmp ^= FIO___AES_BS_RCON[(i >> 3) - 1];
    } else if ((i & 7) == 4) {
      /* SubWord only (AES-256 extra step) */
      tmp = fio___aes_bs_sub_word(tmp);
    }
    skey[i] = skey[i - 8] ^ tmp;
  }

  /* Phase 2: compress into bitsliced format */
  for (i = 0; i < 60; i += 4) {
    uint64_t q[8];
    int j = (i >> 1);
    fio___aes_bs_interleave_in(&q[0], &q[4], skey + i);
    q[1] = q[0];
    q[2] = q[0];
    q[3] = q[0];
    q[5] = q[4];
    q[6] = q[4];
    q[7] = q[4];
    fio___aes_bs_ortho(q);
    comp_skey[j + 0] = (q[0] & (uint64_t)0x1111111111111111ULL) |
                       (q[1] & (uint64_t)0x2222222222222222ULL) |
                       (q[2] & (uint64_t)0x4444444444444444ULL) |
                       (q[3] & (uint64_t)0x8888888888888888ULL);
    comp_skey[j + 1] = (q[4] & (uint64_t)0x1111111111111111ULL) |
                       (q[5] & (uint64_t)0x2222222222222222ULL) |
                       (q[6] & (uint64_t)0x4444444444444444ULL) |
                       (q[7] & (uint64_t)0x8888888888888888ULL);
  }
}

/* ============================================================================
 * 9. Full bitsliced encryption (16-block fio_u256 engine)
 *
 * Uses pre-expanded keys for the 16-block path.
 * For each round: Sbox → ShiftRows → MixColumns → AddRoundKey
 * Last round omits MixColumns.
 * ========================================================================= */

#if FIO___AES_BS_WIDE

/**
 * Encrypt bitsliced state q[8] (fio_u256, 16 blocks) using num_rounds rounds.
 * sk_exp4: pre-expanded round keys (same format as 4-block, broadcast to all
 * lanes).
 */
FIO_SFUNC void fio___aes_bs_encrypt(unsigned num_rounds,
                                    const uint64_t *sk_exp4,
                                    fio_u256 *q) {
  unsigned u;
  fio___aes_bs_add_round_key(q, sk_exp4);
  for (u = 1; u < num_rounds; ++u) {
    fio___aes_bs_sbox(q);
    fio___aes_bs_shift_rows(q);
    fio___aes_bs_mix_columns(q);
    fio___aes_bs_add_round_key(q, sk_exp4 + (u << 3));
  }
  fio___aes_bs_sbox(q);
  fio___aes_bs_shift_rows(q);
  fio___aes_bs_add_round_key(q, sk_exp4 + (num_rounds << 3));
}

#endif /* FIO___AES_BS_WIDE */

/* ============================================================================
 * 10. Constant-time carryless multiply (bmul64) and bit reversal (rev64)
 *
 * Used by GHASH. The "holes in integers" technique: split each operand into
 * 4 groups of every-4th-bit, multiply with regular integer multiply (the
 * 3 zero bits between data bits absorb carries), then mask and recombine.
 * ========================================================================= */

/** Constant-time carryless (polynomial) multiplication of two 64-bit values. */
FIO_IFUNC uint64_t fio___bmul64(uint64_t x, uint64_t y) {
  uint64_t x0, x1, x2, x3;
  uint64_t y0, y1, y2, y3;
  uint64_t z0, z1, z2, z3;

  x0 = x & 0x1111111111111111ULL;
  x1 = x & 0x2222222222222222ULL;
  x2 = x & 0x4444444444444444ULL;
  x3 = x & 0x8888888888888888ULL;
  y0 = y & 0x1111111111111111ULL;
  y1 = y & 0x2222222222222222ULL;
  y2 = y & 0x4444444444444444ULL;
  y3 = y & 0x8888888888888888ULL;

  z0 = (x0 * y0) ^ (x1 * y3) ^ (x2 * y2) ^ (x3 * y1);
  z1 = (x0 * y1) ^ (x1 * y0) ^ (x2 * y3) ^ (x3 * y2);
  z2 = (x0 * y2) ^ (x1 * y1) ^ (x2 * y0) ^ (x3 * y3);
  z3 = (x0 * y3) ^ (x1 * y2) ^ (x2 * y1) ^ (x3 * y0);

  z0 &= 0x1111111111111111ULL;
  z1 &= 0x2222222222222222ULL;
  z2 &= 0x4444444444444444ULL;
  z3 &= 0x8888888888888888ULL;

  return z0 | z1 | z2 | z3;
}

/** Reverse all 64 bits of x. Used for GHASH bit-reversed representation. */
FIO_IFUNC uint64_t fio___rev64(uint64_t x) {
  x = ((x & 0x5555555555555555ULL) << 1) | ((x >> 1) & 0x5555555555555555ULL);
  x = ((x & 0x3333333333333333ULL) << 2) | ((x >> 2) & 0x3333333333333333ULL);
  x = ((x & 0x0F0F0F0F0F0F0F0FULL) << 4) | ((x >> 4) & 0x0F0F0F0F0F0F0F0FULL);
  x = ((x & 0x00FF00FF00FF00FFULL) << 8) | ((x >> 8) & 0x00FF00FF00FF00FFULL);
  x = ((x & 0x0000FFFF0000FFFFULL) << 16) | ((x >> 16) & 0x0000FFFF0000FFFFULL);
  return (x << 32) | (x >> 32);
}

/* ============================================================================
 * 11. Constant-time GHASH multiplication (ctmul64)
 *
 * Full GHASH block multiplication using Karatsuba decomposition + bmul64.
 * Processes one 16-byte block at a time.
 *
 * GHASH accumulator y[0..1] (big-endian halves), hash key h[0..1].
 * Reduction polynomial: x^128 + x^7 + x^2 + x + 1.
 * ========================================================================= */

/**
 * Precomputed GHASH hash key entry for ctmul64.
 * Stores h0, h1 (big-endian halves) and their bit-reversed forms,
 * plus h2 = h0^h1 for Karatsuba.
 */
typedef struct {
  uint64_t h0, h1;   /* hash key halves (big-endian) */
  uint64_t h0r, h1r; /* bit-reversed hash key halves */
  uint64_t h2, h2r;  /* h0 ^ h1 and rev(h0) ^ rev(h1) for Karatsuba */
} fio___ghash_ctmul_entry_s;

/**
 * Precomputed GHASH key table: H^1, H^2, H^3, H^4 for 4-block aggregation.
 * hk[0] = H^4, hk[1] = H^3, hk[2] = H^2, hk[3] = H^1.
 * Ordering: block[0] multiplied by H^4, block[3] by H^1.
 */
typedef struct {
  fio___ghash_ctmul_entry_s hk[4]; /* H^4, H^3, H^2, H^1 */
} fio___ghash_ctmul_key_s;

/** Initialize a single GHASH key entry from h0, h1 values. */
FIO_IFUNC void fio___ghash_ctmul_entry_init(fio___ghash_ctmul_entry_s *e,
                                            uint64_t h0,
                                            uint64_t h1) {
  e->h0 = h0;
  e->h1 = h1;
  e->h0r = fio___rev64(h0);
  e->h1r = fio___rev64(h1);
  e->h2 = h0 ^ h1;
  e->h2r = e->h0r ^ e->h1r;
}

/**
 * GF(2^128) multiply: result = a * b (no XOR with accumulator).
 * Used for computing H powers. Returns result in out[0] (high), out[1] (low).
 */
FIO_SFUNC void fio___ghash_ctmul_gmul(uint64_t out[2],
                                      const uint64_t a[2],
                                      const fio___ghash_ctmul_entry_s *bk) {
  uint64_t a0, a1, a0r, a1r, a2, a2r;
  uint64_t z0, z1, z2, z0h, z1h, z2h;
  uint64_t v0, v1, v2, v3;

  a1 = a[0]; /* high half */
  a0 = a[1]; /* low half */
  a0r = fio___rev64(a0);
  a1r = fio___rev64(a1);
  a2 = a0 ^ a1;
  a2r = a0r ^ a1r;

  z0 = fio___bmul64(a0, bk->h0);
  z1 = fio___bmul64(a1, bk->h1);
  z2 = fio___bmul64(a2, bk->h2);
  z0h = fio___bmul64(a0r, bk->h0r);
  z1h = fio___bmul64(a1r, bk->h1r);
  z2h = fio___bmul64(a2r, bk->h2r);

  z2 ^= z0 ^ z1;
  z2h ^= z0h ^ z1h;
  z0h = fio___rev64(z0h) >> 1;
  z1h = fio___rev64(z1h) >> 1;
  z2h = fio___rev64(z2h) >> 1;

  v0 = z0;
  v1 = z0h ^ z2;
  v2 = z1 ^ z2h;
  v3 = z1h;

  v3 = (v3 << 1) | (v2 >> 63);
  v2 = (v2 << 1) | (v1 >> 63);
  v1 = (v1 << 1) | (v0 >> 63);
  v0 = (v0 << 1);

  v2 ^= v0 ^ (v0 >> 1) ^ (v0 >> 2) ^ (v0 >> 7);
  v1 ^= (v0 << 63) ^ (v0 << 62) ^ (v0 << 57);
  v3 ^= v1 ^ (v1 >> 1) ^ (v1 >> 2) ^ (v1 >> 7);
  v2 ^= (v1 << 63) ^ (v1 << 62) ^ (v1 << 57);

  out[0] = v3;
  out[1] = v2;
}

/** Precompute GHASH key table (H^1 through H^4) for ctmul64.
 * BearSSL convention: h1 = high half (first 8 bytes), h0 = low half. */
FIO_IFUNC void fio___ghash_ctmul_precompute(fio___ghash_ctmul_key_s *key,
                                            const uint8_t h[16]) {
  uint64_t h1 = fio_buf2u64_be(h);     /* high half (first 8 bytes) */
  uint64_t h0 = fio_buf2u64_be(h + 8); /* low half (last 8 bytes) */

  /* H^1 stored at index 3 (used for last block in 4-block batch) */
  fio___ghash_ctmul_entry_init(&key->hk[3], h0, h1);

  /* H^2 = H * H */
  uint64_t hp[2] = {h1, h0}; /* {high, low} */
  uint64_t h2v[2];
  fio___ghash_ctmul_gmul(h2v, hp, &key->hk[3]);
  fio___ghash_ctmul_entry_init(&key->hk[2], h2v[1], h2v[0]);

  /* H^3 = H^2 * H */
  uint64_t h3v[2];
  fio___ghash_ctmul_gmul(h3v, h2v, &key->hk[3]);
  fio___ghash_ctmul_entry_init(&key->hk[1], h3v[1], h3v[0]);

  /* H^4 = H^3 * H */
  uint64_t h4v[2];
  fio___ghash_ctmul_gmul(h4v, h3v, &key->hk[3]);
  fio___ghash_ctmul_entry_init(&key->hk[0], h4v[1], h4v[0]);
}

/**
 * GHASH multiply: y = (y XOR block) * H
 *
 * Uses Karatsuba decomposition (3 bmul64 calls instead of 4) with
 * bit-reversal for the GHASH reflected representation.
 */
FIO_SFUNC void fio___ghash_ctmul(uint64_t y[2],
                                 const uint8_t block[16],
                                 const fio___ghash_ctmul_key_s *key) {
  const fio___ghash_ctmul_entry_s *hk = &key->hk[3]; /* H^1 */
  uint64_t y0, y1, y0r, y1r, y2, y2r;
  uint64_t z0, z1, z2, z0h, z1h, z2h;
  uint64_t v0, v1, v2, v3;

  /* XOR input block into accumulator */
  y1 = y[0] ^ fio_buf2u64_be(block);
  y0 = y[1] ^ fio_buf2u64_be(block + 8);

  /* Bit-reverse accumulator halves */
  y0r = fio___rev64(y0);
  y1r = fio___rev64(y1);
  y2 = y0 ^ y1;
  y2r = y0r ^ y1r;

  /* Three Karatsuba multiplications (forward and reversed) */
  z0 = fio___bmul64(y0, hk->h0);
  z1 = fio___bmul64(y1, hk->h1);
  z2 = fio___bmul64(y2, hk->h2);
  z0h = fio___bmul64(y0r, hk->h0r);
  z1h = fio___bmul64(y1r, hk->h1r);
  z2h = fio___bmul64(y2r, hk->h2r);

  /* Karatsuba combination */
  z2 ^= z0 ^ z1;
  z2h ^= z0h ^ z1h;

  /* Reverse the "high" halves to recover missing bits */
  z0h = fio___rev64(z0h) >> 1;
  z1h = fio___rev64(z1h) >> 1;
  z2h = fio___rev64(z2h) >> 1;

  /* Assemble 256-bit product */
  v0 = z0;
  v1 = z0h ^ z2;
  v2 = z1 ^ z2h;
  v3 = z1h;

  /* Left shift by 1 (GHASH bit-reversed representation) */
  v3 = (v3 << 1) | (v2 >> 63);
  v2 = (v2 << 1) | (v1 >> 63);
  v1 = (v1 << 1) | (v0 >> 63);
  v0 = (v0 << 1);

  /* Reduction modulo x^128 + x^7 + x^2 + x + 1 */
  /* Reduce v0 */
  v2 ^= v0 ^ (v0 >> 1) ^ (v0 >> 2) ^ (v0 >> 7);
  v1 ^= (v0 << 63) ^ (v0 << 62) ^ (v0 << 57);
  /* Reduce v1 */
  v3 ^= v1 ^ (v1 >> 1) ^ (v1 >> 2) ^ (v1 >> 7);
  v2 ^= (v1 << 63) ^ (v1 << 62) ^ (v1 << 57);

  /* Result in v2:v3 */
  y[0] = v3;
  y[1] = v2;
}

/**
 * Aggregated 4-block GHASH: y = (y XOR C0) * H^4 XOR C1 * H^3 XOR C2 * H^2
 * XOR C3 * H^1
 *
 * Fully unrolled with maximally deferred post-processing:
 * - 4 Karatsuba multiplications (24 bmul64 + 8 rev64 for inputs)
 * - Accumulate raw z0/z1/z2/z0h/z1h/z2h across all 4 blocks
 * - Single Karatsuba combination + 3 rev64 + assembly + shift + reduction
 * Saves 9 rev64 calls and 3 reductions vs 4 separate single-block calls.
 *
 * blocks must point to exactly 64 bytes (4 × 16-byte blocks).
 */
FIO_SFUNC void fio___ghash_ctmul4(uint64_t y[2],
                                  const uint8_t blocks[64],
                                  const fio___ghash_ctmul_key_s *key) {
  uint64_t sz0, sz1, sz2, sz0h, sz1h, sz2h;
  uint64_t v0, v1, v2, v3;

  /* --- Block 0: (y XOR C0) * H^4 --- */
  {
    const fio___ghash_ctmul_entry_s *hk = &key->hk[0];
    uint64_t a1 = y[0] ^ fio_buf2u64_be(blocks);
    uint64_t a0 = y[1] ^ fio_buf2u64_be(blocks + 8);
    uint64_t a0r = fio___rev64(a0);
    uint64_t a1r = fio___rev64(a1);
    uint64_t a2 = a0 ^ a1;
    uint64_t a2r = a0r ^ a1r;
    sz0 = fio___bmul64(a0, hk->h0);
    sz1 = fio___bmul64(a1, hk->h1);
    sz2 = fio___bmul64(a2, hk->h2);
    sz0h = fio___bmul64(a0r, hk->h0r);
    sz1h = fio___bmul64(a1r, hk->h1r);
    sz2h = fio___bmul64(a2r, hk->h2r);
  }

  /* --- Block 1: C1 * H^3 --- */
  {
    const fio___ghash_ctmul_entry_s *hk = &key->hk[1];
    uint64_t a1 = fio_buf2u64_be(blocks + 16);
    uint64_t a0 = fio_buf2u64_be(blocks + 24);
    uint64_t a0r = fio___rev64(a0);
    uint64_t a1r = fio___rev64(a1);
    uint64_t a2 = a0 ^ a1;
    uint64_t a2r = a0r ^ a1r;
    sz0 ^= fio___bmul64(a0, hk->h0);
    sz1 ^= fio___bmul64(a1, hk->h1);
    sz2 ^= fio___bmul64(a2, hk->h2);
    sz0h ^= fio___bmul64(a0r, hk->h0r);
    sz1h ^= fio___bmul64(a1r, hk->h1r);
    sz2h ^= fio___bmul64(a2r, hk->h2r);
  }

  /* --- Block 2: C2 * H^2 --- */
  {
    const fio___ghash_ctmul_entry_s *hk = &key->hk[2];
    uint64_t a1 = fio_buf2u64_be(blocks + 32);
    uint64_t a0 = fio_buf2u64_be(blocks + 40);
    uint64_t a0r = fio___rev64(a0);
    uint64_t a1r = fio___rev64(a1);
    uint64_t a2 = a0 ^ a1;
    uint64_t a2r = a0r ^ a1r;
    sz0 ^= fio___bmul64(a0, hk->h0);
    sz1 ^= fio___bmul64(a1, hk->h1);
    sz2 ^= fio___bmul64(a2, hk->h2);
    sz0h ^= fio___bmul64(a0r, hk->h0r);
    sz1h ^= fio___bmul64(a1r, hk->h1r);
    sz2h ^= fio___bmul64(a2r, hk->h2r);
  }

  /* --- Block 3: C3 * H^1 --- */
  {
    const fio___ghash_ctmul_entry_s *hk = &key->hk[3];
    uint64_t a1 = fio_buf2u64_be(blocks + 48);
    uint64_t a0 = fio_buf2u64_be(blocks + 56);
    uint64_t a0r = fio___rev64(a0);
    uint64_t a1r = fio___rev64(a1);
    uint64_t a2 = a0 ^ a1;
    uint64_t a2r = a0r ^ a1r;
    sz0 ^= fio___bmul64(a0, hk->h0);
    sz1 ^= fio___bmul64(a1, hk->h1);
    sz2 ^= fio___bmul64(a2, hk->h2);
    sz0h ^= fio___bmul64(a0r, hk->h0r);
    sz1h ^= fio___bmul64(a1r, hk->h1r);
    sz2h ^= fio___bmul64(a2r, hk->h2r);
  }

  /* Single Karatsuba combination on accumulated sums */
  sz2 ^= sz0 ^ sz1;
  sz2h ^= sz0h ^ sz1h;

  /* Single set of 3 rev64 calls (instead of 4 × 3 = 12) */
  sz0h = fio___rev64(sz0h) >> 1;
  sz1h = fio___rev64(sz1h) >> 1;
  sz2h = fio___rev64(sz2h) >> 1;

  /* Assemble 256-bit product */
  v0 = sz0;
  v1 = sz0h ^ sz2;
  v2 = sz1 ^ sz2h;
  v3 = sz1h;

  /* Single left shift by 1 on the accumulated 256-bit product */
  v3 = (v3 << 1) | (v2 >> 63);
  v2 = (v2 << 1) | (v1 >> 63);
  v1 = (v1 << 1) | (v0 >> 63);
  v0 = (v0 << 1);

  /* Single reduction modulo x^128 + x^7 + x^2 + x + 1 */
  v2 ^= v0 ^ (v0 >> 1) ^ (v0 >> 2) ^ (v0 >> 7);
  v1 ^= (v0 << 63) ^ (v0 << 62) ^ (v0 << 57);
  v3 ^= v1 ^ (v1 >> 1) ^ (v1 >> 2) ^ (v1 >> 7);
  v2 ^= (v1 << 63) ^ (v1 << 62) ^ (v1 << 57);

  y[0] = v3;
  y[1] = v2;
}

/* ============================================================================
 * 12-14. CTR counter block preparation and encryption
 *
 * 16-block engine (fio_u256) for large data, 4-block engine for tail.
 * ========================================================================= */

#if FIO___AES_BS_WIDE

/** Prepare 16 counter blocks for bitsliced CTR mode. */
FIO_SFUNC void fio___aes_bs_ctr_prepare16(uint32_t w[64],
                                          const uint32_t nonce_w[3],
                                          uint32_t ctr_start) {
  for (unsigned b = 0; b < 16; ++b) {
    w[b * 4 + 0] = nonce_w[0];
    w[b * 4 + 1] = nonce_w[1];
    w[b * 4 + 2] = nonce_w[2];
    w[b * 4 + 3] = fio_lton32(ctr_start + b);
  }
}

/** Encrypt 16 counter blocks and produce 256 bytes of keystream. */
FIO_SFUNC void fio___aes_bs_ctr_encrypt16(uint8_t ks[256],
                                          const uint32_t w[64],
                                          unsigned num_rounds,
                                          const uint64_t *sk_exp4) {
  fio_u256 q[8];
  uint64_t tmp[8];

  for (unsigned g = 0; g < 4; ++g) {
    for (unsigned b = 0; b < 4; ++b) {
      unsigned blk_idx = g * 4 + b;
      fio___aes_bs_interleave_in(&q[b].u64[g],
                                 &q[b + 4].u64[g],
                                 w + blk_idx * 4);
    }
  }

  for (size_t lane = 0; lane < 4; ++lane) {
    for (int i = 0; i < 8; ++i)
      tmp[i] = q[i].u64[lane];
    fio___aes_bs_ortho(tmp);
    for (int i = 0; i < 8; ++i)
      q[i].u64[lane] = tmp[i];
  }

  fio___aes_bs_encrypt(num_rounds, sk_exp4, q);

  for (size_t lane = 0; lane < 4; ++lane) {
    for (int i = 0; i < 8; ++i)
      tmp[i] = q[i].u64[lane];
    fio___aes_bs_ortho(tmp);
    for (int i = 0; i < 8; ++i)
      q[i].u64[lane] = tmp[i];
  }

  for (unsigned g = 0; g < 4; ++g) {
    for (unsigned b = 0; b < 4; ++b) {
      unsigned blk_idx = g * 4 + b;
      uint32_t out_w[4];
      fio___aes_bs_interleave_out(out_w, q[b].u64[g], q[b + 4].u64[g]);
      fio_u2buf32_le(ks + blk_idx * 16, out_w[0]);
      fio_u2buf32_le(ks + blk_idx * 16 + 4, out_w[1]);
      fio_u2buf32_le(ks + blk_idx * 16 + 8, out_w[2]);
      fio_u2buf32_le(ks + blk_idx * 16 + 12, out_w[3]);
    }
  }
}

#endif /* FIO___AES_BS_WIDE */

/* ============================================================================
 * 15. GHASH helpers for GCM
 * ========================================================================= */

/** GHASH over arbitrary-length data (full blocks + optional partial).
 * Uses 4-block aggregation for ≥64 bytes, single-block for tail. */
FIO_SFUNC void fio___aes_bs_ghash(uint64_t y[2],
                                  const fio___ghash_ctmul_key_s *hk,
                                  const uint8_t *data,
                                  size_t len) {
  while (len >= 64) {
    fio___ghash_ctmul4(y, data, hk);
    data += 64;
    len -= 64;
  }
  while (len >= 16) {
    fio___ghash_ctmul(y, data, hk);
    data += 16;
    len -= 16;
  }
  if (len > 0) {
    uint8_t block[16] = {0};
    FIO_MEMCPY(block, data, len);
    fio___ghash_ctmul(y, block, hk);
  }
}

/* ============================================================================
 * 16. Shared GCM CTR processing (encrypt/decrypt XOR loop)
 *
 * Uses 16-block engine for >=256B chunks (when FIO___AES_BS_WIDE=1),
 * then 4-block engine for remaining data.
 * ========================================================================= */

/**
 * CTR-mode XOR processing with optional GHASH of output.
 * If ghash_output is true, GHASH is applied to the data AFTER XOR (encryption).
 * If ghash_output is false, no GHASH is done here (decryption does it
 * separately).
 */
FIO_SFUNC void fio___aes_bs_gcm_ctr_process(uint8_t *p,
                                            size_t len,
                                            const uint32_t nonce_w[3],
                                            uint32_t *ctr,
                                            unsigned num_rounds,
                                            const uint64_t *sk_exp4,
                                            uint64_t ghash_y[2],
                                            const fio___ghash_ctmul_key_s *hk,
                                            int ghash_output) {
#if FIO___AES_BS_WIDE
  /* Process 16 blocks (256 bytes) at a time using wide engine */
  while (len >= 256) {
    uint32_t w[64];
    uint8_t ks[256];
    fio___aes_bs_ctr_prepare16(w, nonce_w, *ctr);
    fio___aes_bs_ctr_encrypt16(ks, w, num_rounds, sk_exp4);
    for (size_t i = 0; i < 256; i += 8) {
      uint64_t d, k;
      FIO_MEMCPY(&d, p + i, 8);
      FIO_MEMCPY(&k, ks + i, 8);
      d ^= k;
      FIO_MEMCPY(p + i, &d, 8);
    }
    if (ghash_output) {
      for (size_t i = 0; i < 256; i += 64)
        fio___ghash_ctmul4(ghash_y, p + i, hk);
    }
    *ctr += 16;
    p += 256;
    len -= 256;
  }
#endif /* FIO___AES_BS_WIDE */

  /* Process 4 blocks (64 bytes) at a time using 4-block engine */
  while (len >= 64) {
    uint32_t w4[16];
    uint8_t ks4[64];
    fio___aes_bs_ctr_prepare4(w4, nonce_w, *ctr);
    fio___aes_bs_ctr_encrypt4_ks(ks4, w4, num_rounds, sk_exp4);
    for (size_t i = 0; i < 64; i += 8) {
      uint64_t d, k;
      FIO_MEMCPY(&d, p + i, 8);
      FIO_MEMCPY(&k, ks4 + i, 8);
      d ^= k;
      FIO_MEMCPY(p + i, &d, 8);
    }
    if (ghash_output) {
      fio___ghash_ctmul4(ghash_y, p, hk);
    }
    *ctr += 4;
    p += 64;
    len -= 64;
  }

  /* Handle remaining data (< 64 bytes) using 4-block engine */
  if (len > 0) {
    uint32_t w4[16];
    uint8_t ks4[64];
    fio___aes_bs_ctr_prepare4(w4, nonce_w, *ctr);
    fio___aes_bs_ctr_encrypt4_ks(ks4, w4, num_rounds, sk_exp4);

    size_t full_blocks = len & ~(size_t)15;
    for (size_t i = 0; i < full_blocks; i += 8) {
      uint64_t d, k;
      FIO_MEMCPY(&d, p + i, 8);
      FIO_MEMCPY(&k, ks4 + i, 8);
      d ^= k;
      FIO_MEMCPY(p + i, &d, 8);
    }
    if (ghash_output) {
      for (size_t i = 0; i < full_blocks; i += 16)
        fio___ghash_ctmul(ghash_y, p + i, hk);
    }

    size_t tail = len - full_blocks;
    if (tail > 0) {
      for (size_t i = 0; i < tail; ++i)
        p[full_blocks + i] ^= ks4[full_blocks + i];
      if (ghash_output) {
        uint8_t block[16] = {0};
        FIO_MEMCPY(block, p + full_blocks, tail);
        fio___ghash_ctmul(ghash_y, block, hk);
      }
    }
    *ctr += (uint32_t)((len + 15) >> 4);
  }
}

/* ============================================================================
 * 17. AES-GCM public API (bitsliced constant-time implementation)
 *
 * Fused small-message path: for messages ≤ 64 bytes, H + J0 + first 2 CTR
 * blocks are batched in a single 4-block engine call, saving one full
 * ortho+interleave round-trip vs the separate init + CTR approach.
 * ========================================================================= */

/**
 * Shared GCM init: key expand + pre-expand keys + batch H & J0 computation.
 * Returns j0_enc[16] (encrypted J0 for final tag).
 */
FIO_SFUNC void fio___aes_bs_gcm_init(uint64_t *sk_exp4,
                                     fio___ghash_ctmul_key_s *hk,
                                     uint8_t j0_enc[16],
                                     uint32_t nonce_w[3],
                                     unsigned num_rounds,
                                     const uint64_t *comp_skey,
                                     const void *nonce) {
  uint32_t h_w[4] = {0}, j0_w[4];
  uint32_t h_out[4], j0_out[4];
  uint8_t h_bytes[16];

  /* Pre-expand all round keys once */
  fio___aes_bs_expand_all_keys4(sk_exp4, num_rounds, comp_skey);

  /* Nonce words */
  nonce_w[0] = fio_buf2u32_le((const uint8_t *)nonce);
  nonce_w[1] = fio_buf2u32_le((const uint8_t *)nonce + 4);
  nonce_w[2] = fio_buf2u32_le((const uint8_t *)nonce + 8);

  /* Batch H + J0: encrypt zeros (for H) and nonce||1 (for J0) in one call */
  j0_w[0] = nonce_w[0];
  j0_w[1] = nonce_w[1];
  j0_w[2] = nonce_w[2];
  j0_w[3] = fio_lton32(1);
  fio___aes_bs_encrypt_2blocks(h_out, j0_out, h_w, j0_w, num_rounds, sk_exp4);

  /* H -> GHASH key */
  fio_u2buf32_le(h_bytes, h_out[0]);
  fio_u2buf32_le(h_bytes + 4, h_out[1]);
  fio_u2buf32_le(h_bytes + 8, h_out[2]);
  fio_u2buf32_le(h_bytes + 12, h_out[3]);
  fio___ghash_ctmul_precompute(hk, h_bytes);

  /* J0 -> encrypted for final tag */
  fio_u2buf32_le(j0_enc, j0_out[0]);
  fio_u2buf32_le(j0_enc + 4, j0_out[1]);
  fio_u2buf32_le(j0_enc + 8, j0_out[2]);
  fio_u2buf32_le(j0_enc + 12, j0_out[3]);

  fio_secure_zero(h_bytes, sizeof(h_bytes));
}

/**
 * Fused GCM init + first 2 CTR blocks: encrypts H, J0, CTR[2], CTR[3] in a
 * single 4-block engine call using compressed keys (no sk_exp4 needed).
 * Returns 32 bytes of keystream in ks_out[0..31].
 * For messages ≤ 32 bytes, this is the only engine call needed for CTR.
 */
FIO_SFUNC void fio___aes_bs_gcm_init_with_ctr(fio___ghash_ctmul_key_s *hk,
                                              uint8_t j0_enc[16],
                                              uint8_t ks_out[32],
                                              uint32_t nonce_w[3],
                                              unsigned num_rounds,
                                              const uint64_t *comp_skey,
                                              const void *nonce) {
  uint32_t h_w[4] = {0}, j0_w[4], ctr2_w[4], ctr3_w[4];
  uint64_t q[8];
  uint8_t h_bytes[16];

  /* Nonce words */
  nonce_w[0] = fio_buf2u32_le((const uint8_t *)nonce);
  nonce_w[1] = fio_buf2u32_le((const uint8_t *)nonce + 4);
  nonce_w[2] = fio_buf2u32_le((const uint8_t *)nonce + 8);

  /* Prepare 4 blocks: H (zeros), J0 (nonce||1), CTR[2], CTR[3] */
  j0_w[0] = nonce_w[0];
  j0_w[1] = nonce_w[1];
  j0_w[2] = nonce_w[2];
  j0_w[3] = fio_lton32(1);
  ctr2_w[0] = nonce_w[0];
  ctr2_w[1] = nonce_w[1];
  ctr2_w[2] = nonce_w[2];
  ctr2_w[3] = fio_lton32(2);
  ctr3_w[0] = nonce_w[0];
  ctr3_w[1] = nonce_w[1];
  ctr3_w[2] = nonce_w[2];
  ctr3_w[3] = fio_lton32(3);

  /* Pack all 4 blocks into q[8] */
  fio___aes_bs_interleave_in(&q[0], &q[4], h_w);
  fio___aes_bs_interleave_in(&q[1], &q[5], j0_w);
  fio___aes_bs_interleave_in(&q[2], &q[6], ctr2_w);
  fio___aes_bs_interleave_in(&q[3], &q[7], ctr3_w);

  /* Single engine call for all 4 blocks, on-the-fly key expansion */
  fio___aes_bs_ortho(q);
  fio___aes_bs_encrypt4_comp(num_rounds, comp_skey, q);
  fio___aes_bs_ortho(q);

  /* Extract H (slot 0) */
  {
    uint32_t h_out[4];
    fio___aes_bs_interleave_out(h_out, q[0], q[4]);
    fio_u2buf32_le(h_bytes, h_out[0]);
    fio_u2buf32_le(h_bytes + 4, h_out[1]);
    fio_u2buf32_le(h_bytes + 8, h_out[2]);
    fio_u2buf32_le(h_bytes + 12, h_out[3]);
    fio___ghash_ctmul_precompute(hk, h_bytes);
  }

  /* Extract J0 (slot 1) */
  {
    uint32_t j0_out[4];
    fio___aes_bs_interleave_out(j0_out, q[1], q[5]);
    fio_u2buf32_le(j0_enc, j0_out[0]);
    fio_u2buf32_le(j0_enc + 4, j0_out[1]);
    fio_u2buf32_le(j0_enc + 8, j0_out[2]);
    fio_u2buf32_le(j0_enc + 12, j0_out[3]);
  }

  /* Extract CTR[2] keystream (slot 2) -> ks_out[0..15] */
  {
    uint32_t out_w[4];
    fio___aes_bs_interleave_out(out_w, q[2], q[6]);
    fio_u2buf32_le(ks_out, out_w[0]);
    fio_u2buf32_le(ks_out + 4, out_w[1]);
    fio_u2buf32_le(ks_out + 8, out_w[2]);
    fio_u2buf32_le(ks_out + 12, out_w[3]);
  }

  /* Extract CTR[3] keystream (slot 3) -> ks_out[16..31] */
  {
    uint32_t out_w[4];
    fio___aes_bs_interleave_out(out_w, q[3], q[7]);
    fio_u2buf32_le(ks_out + 16, out_w[0]);
    fio_u2buf32_le(ks_out + 20, out_w[1]);
    fio_u2buf32_le(ks_out + 24, out_w[2]);
    fio_u2buf32_le(ks_out + 28, out_w[3]);
  }

  fio_secure_zero(h_bytes, sizeof(h_bytes));
}

/**
 * Compute GHASH length block and final tag, XOR with j0_enc.
 * Writes 16-byte tag to mac_out.
 */
FIO_IFUNC void fio___aes_bs_gcm_finalize(uint8_t *mac_out,
                                         uint64_t ghash_y[2],
                                         const fio___ghash_ctmul_key_s *hk,
                                         const uint8_t j0_enc[16],
                                         uint64_t adlen,
                                         uint64_t datalen) {
  uint8_t len_block[16];
  fio_u2buf64_be(len_block, adlen * 8);
  fio_u2buf64_be(len_block + 8, datalen * 8);
  fio___ghash_ctmul(ghash_y, len_block, hk);

  fio_u2buf64_be(mac_out, ghash_y[0]);
  fio_u2buf64_be(mac_out + 8, ghash_y[1]);
  {
    uint64_t t0, t1, j0, j1;
    FIO_MEMCPY(&t0, mac_out, 8);
    FIO_MEMCPY(&j0, j0_enc, 8);
    t0 ^= j0;
    FIO_MEMCPY(mac_out, &t0, 8);
    FIO_MEMCPY(&t1, mac_out + 8, 8);
    FIO_MEMCPY(&j1, j0_enc + 8, 8);
    t1 ^= j1;
    FIO_MEMCPY(mac_out + 8, &t1, 8);
  }
}

/**
 * Fused AES-GCM encrypt for small messages (≤ 64 bytes).
 * Batches H + J0 + first 2 CTR blocks in one 4-block engine call.
 * For ≤ 32 bytes: 1 engine call total. For 33-64 bytes: 2 engine calls.
 * Returns via enc_fn pointer pattern for code sharing between 128/256.
 */
FIO_SFUNC void fio___aes_bs_gcm_enc_small(void *restrict mac,
                                          uint8_t *restrict p,
                                          size_t len,
                                          const void *ad,
                                          size_t adlen,
                                          unsigned num_rounds,
                                          const uint64_t *comp_skey,
                                          const void *nonce) {
  fio___ghash_ctmul_key_s hk;
  uint64_t ghash_y[2] = {0, 0};
  uint32_t nonce_w[3];
  uint8_t j0_enc[16];
  uint8_t ks[96]; /* 32 from init + up to 64 from second call */

  /* Init + first 2 CTR blocks in one engine call (no sk_exp4 needed) */
  fio___aes_bs_gcm_init_with_ctr(&hk,
                                 j0_enc,
                                 ks,
                                 nonce_w,
                                 num_rounds,
                                 comp_skey,
                                 nonce);

  /* If we need more than 32 bytes of keystream, generate CTR[4..7] */
  if (len > 32) {
    uint32_t w4[16];
    uint64_t q[8];
    fio___aes_bs_ctr_prepare4(w4, nonce_w, 4);
    /* Use on-the-fly key expansion for second engine call too */
    for (unsigned b = 0; b < 4; ++b)
      fio___aes_bs_interleave_in(&q[b], &q[b + 4], w4 + b * 4);
    fio___aes_bs_ortho(q);
    fio___aes_bs_encrypt4_comp(num_rounds, comp_skey, q);
    fio___aes_bs_ortho(q);
    for (unsigned b = 0; b < 4; ++b) {
      uint32_t out_w[4];
      fio___aes_bs_interleave_out(out_w, q[b], q[b + 4]);
      fio_u2buf32_le(ks + 32 + b * 16, out_w[0]);
      fio_u2buf32_le(ks + 32 + b * 16 + 4, out_w[1]);
      fio_u2buf32_le(ks + 32 + b * 16 + 8, out_w[2]);
      fio_u2buf32_le(ks + 32 + b * 16 + 12, out_w[3]);
    }
  }

  /* GHASH AAD */
  fio___aes_bs_ghash(ghash_y, &hk, (const uint8_t *)ad, adlen);

  /* XOR keystream with data + GHASH ciphertext */
  {
    size_t full_blocks = len & ~(size_t)15;
    for (size_t i = 0; i < full_blocks; i += 8) {
      uint64_t d, k;
      FIO_MEMCPY(&d, p + i, 8);
      FIO_MEMCPY(&k, ks + i, 8);
      d ^= k;
      FIO_MEMCPY(p + i, &d, 8);
    }
    for (size_t i = 0; i < full_blocks; i += 16)
      fio___ghash_ctmul(ghash_y, p + i, &hk);

    size_t tail = len - full_blocks;
    if (tail > 0) {
      for (size_t i = 0; i < tail; ++i)
        p[full_blocks + i] ^= ks[full_blocks + i];
      uint8_t block[16] = {0};
      FIO_MEMCPY(block, p + full_blocks, tail);
      fio___ghash_ctmul(ghash_y, block, &hk);
    }
  }

  /* Finalize tag */
  fio___aes_bs_gcm_finalize((uint8_t *)mac,
                            ghash_y,
                            &hk,
                            j0_enc,
                            (uint64_t)adlen,
                            (uint64_t)len);

  fio_secure_zero(&hk, sizeof(hk));
  fio_secure_zero(j0_enc, sizeof(j0_enc));
  fio_secure_zero(ks, sizeof(ks));
}

/**
 * Fused AES-GCM decrypt for small messages (≤ 64 bytes).
 * Same batching strategy as encrypt.
 */
FIO_SFUNC int fio___aes_bs_gcm_dec_small(void *restrict mac,
                                         uint8_t *restrict p,
                                         size_t len,
                                         const void *ad,
                                         size_t adlen,
                                         unsigned num_rounds,
                                         const uint64_t *comp_skey,
                                         const void *nonce) {
  fio___ghash_ctmul_key_s hk;
  uint64_t ghash_y[2] = {0, 0};
  uint32_t nonce_w[3];
  uint8_t j0_enc[16];
  uint8_t ks[96];
  int result = 0;

  /* Init + first 2 CTR blocks in one engine call (no sk_exp4 needed) */
  fio___aes_bs_gcm_init_with_ctr(&hk,
                                 j0_enc,
                                 ks,
                                 nonce_w,
                                 num_rounds,
                                 comp_skey,
                                 nonce);

  /* GHASH AAD + ciphertext + length block, then verify tag */
  fio___aes_bs_ghash(ghash_y, &hk, (const uint8_t *)ad, adlen);
  fio___aes_bs_ghash(ghash_y, &hk, p, len);

  {
    uint8_t computed_mac[16];
    fio___aes_bs_gcm_finalize(computed_mac,
                              ghash_y,
                              &hk,
                              j0_enc,
                              (uint64_t)adlen,
                              (uint64_t)len);
    if (!fio_ct_is_eq(computed_mac, mac, 16)) {
      result = -1;
      goto cleanup;
    }
  }

  /* Generate remaining keystream if needed */
  if (len > 32) {
    uint32_t w4[16];
    uint64_t q[8];
    fio___aes_bs_ctr_prepare4(w4, nonce_w, 4);
    for (unsigned b = 0; b < 4; ++b)
      fio___aes_bs_interleave_in(&q[b], &q[b + 4], w4 + b * 4);
    fio___aes_bs_ortho(q);
    fio___aes_bs_encrypt4_comp(num_rounds, comp_skey, q);
    fio___aes_bs_ortho(q);
    for (unsigned b = 0; b < 4; ++b) {
      uint32_t out_w[4];
      fio___aes_bs_interleave_out(out_w, q[b], q[b + 4]);
      fio_u2buf32_le(ks + 32 + b * 16, out_w[0]);
      fio_u2buf32_le(ks + 32 + b * 16 + 4, out_w[1]);
      fio_u2buf32_le(ks + 32 + b * 16 + 8, out_w[2]);
      fio_u2buf32_le(ks + 32 + b * 16 + 12, out_w[3]);
    }
  }

  /* XOR keystream with ciphertext to get plaintext */
  {
    size_t full_blocks = len & ~(size_t)15;
    for (size_t i = 0; i < full_blocks; i += 8) {
      uint64_t d, k;
      FIO_MEMCPY(&d, p + i, 8);
      FIO_MEMCPY(&k, ks + i, 8);
      d ^= k;
      FIO_MEMCPY(p + i, &d, 8);
    }
    size_t tail = len - full_blocks;
    if (tail > 0) {
      for (size_t i = 0; i < tail; ++i)
        p[full_blocks + i] ^= ks[full_blocks + i];
    }
  }

cleanup:
  fio_secure_zero(&hk, sizeof(hk));
  fio_secure_zero(j0_enc, sizeof(j0_enc));
  fio_secure_zero(ks, sizeof(ks));
  return result;
}

SFUNC void fio_aes128_gcm_enc(void *restrict mac,
                              void *restrict data,
                              size_t len,
                              const void *ad,
                              size_t adlen,
                              const void *key,
                              const void *nonce) {
  uint64_t comp_skey[22];
  fio___aes_bs_key_expand128(comp_skey, (const uint8_t *)key);

  if (len <= 64 && adlen <= 16) {
    fio___aes_bs_gcm_enc_small(mac,
                               (uint8_t *)data,
                               len,
                               ad,
                               adlen,
                               10,
                               comp_skey,
                               nonce);
    fio_secure_zero(comp_skey, sizeof(comp_skey));
    return;
  }

  {
    uint64_t sk_exp4[11 * 8]; /* (10+1) rounds × 8 */
    fio___ghash_ctmul_key_s hk;
    uint64_t ghash_y[2] = {0, 0};
    uint32_t nonce_w[3];
    uint8_t j0_enc[16];
    uint32_t ctr;

    fio___aes_bs_gcm_init(sk_exp4, &hk, j0_enc, nonce_w, 10, comp_skey, nonce);

    fio___aes_bs_ghash(ghash_y, &hk, (const uint8_t *)ad, adlen);

    ctr = 2;
    fio___aes_bs_gcm_ctr_process((uint8_t *)data,
                                 len,
                                 nonce_w,
                                 &ctr,
                                 10,
                                 sk_exp4,
                                 ghash_y,
                                 &hk,
                                 1);

    fio___aes_bs_gcm_finalize((uint8_t *)mac,
                              ghash_y,
                              &hk,
                              j0_enc,
                              (uint64_t)adlen,
                              (uint64_t)len);

    fio_secure_zero(sk_exp4, sizeof(sk_exp4));
    fio_secure_zero(&hk, sizeof(hk));
    fio_secure_zero(j0_enc, sizeof(j0_enc));
  }

  fio_secure_zero(comp_skey, sizeof(comp_skey));
}

SFUNC void fio_aes256_gcm_enc(void *restrict mac,
                              void *restrict data,
                              size_t len,
                              const void *ad,
                              size_t adlen,
                              const void *key,
                              const void *nonce) {
  uint64_t comp_skey[30];
  fio___aes_bs_key_expand256(comp_skey, (const uint8_t *)key);

  if (len <= 64 && adlen <= 16) {
    fio___aes_bs_gcm_enc_small(mac,
                               (uint8_t *)data,
                               len,
                               ad,
                               adlen,
                               14,
                               comp_skey,
                               nonce);
    fio_secure_zero(comp_skey, sizeof(comp_skey));
    return;
  }

  {
    uint64_t sk_exp4[15 * 8]; /* (14+1) rounds × 8 */
    fio___ghash_ctmul_key_s hk;
    uint64_t ghash_y[2] = {0, 0};
    uint32_t nonce_w[3];
    uint8_t j0_enc[16];
    uint32_t ctr;

    fio___aes_bs_gcm_init(sk_exp4, &hk, j0_enc, nonce_w, 14, comp_skey, nonce);

    fio___aes_bs_ghash(ghash_y, &hk, (const uint8_t *)ad, adlen);

    ctr = 2;
    fio___aes_bs_gcm_ctr_process((uint8_t *)data,
                                 len,
                                 nonce_w,
                                 &ctr,
                                 14,
                                 sk_exp4,
                                 ghash_y,
                                 &hk,
                                 1);

    fio___aes_bs_gcm_finalize((uint8_t *)mac,
                              ghash_y,
                              &hk,
                              j0_enc,
                              (uint64_t)adlen,
                              (uint64_t)len);

    fio_secure_zero(sk_exp4, sizeof(sk_exp4));
    fio_secure_zero(&hk, sizeof(hk));
    fio_secure_zero(j0_enc, sizeof(j0_enc));
  }

  fio_secure_zero(comp_skey, sizeof(comp_skey));
}

SFUNC int fio_aes128_gcm_dec(void *restrict mac,
                             void *restrict data,
                             size_t len,
                             const void *ad,
                             size_t adlen,
                             const void *key,
                             const void *nonce) {
  uint64_t comp_skey[22];
  fio___aes_bs_key_expand128(comp_skey, (const uint8_t *)key);

  if (len <= 64 && adlen <= 16) {
    int ret = fio___aes_bs_gcm_dec_small(mac,
                                         (uint8_t *)data,
                                         len,
                                         ad,
                                         adlen,
                                         10,
                                         comp_skey,
                                         nonce);
    fio_secure_zero(comp_skey, sizeof(comp_skey));
    return ret;
  }

  {
    uint64_t sk_exp4[11 * 8];
    fio___ghash_ctmul_key_s hk;
    uint64_t ghash_y[2] = {0, 0};
    uint32_t nonce_w[3];
    uint8_t j0_enc[16];
    uint32_t ctr;

    fio___aes_bs_gcm_init(sk_exp4, &hk, j0_enc, nonce_w, 10, comp_skey, nonce);

    fio___aes_bs_ghash(ghash_y, &hk, (const uint8_t *)ad, adlen);
    fio___aes_bs_ghash(ghash_y, &hk, (const uint8_t *)data, len);

    {
      uint8_t computed_mac[16];
      fio___aes_bs_gcm_finalize(computed_mac,
                                ghash_y,
                                &hk,
                                j0_enc,
                                (uint64_t)adlen,
                                (uint64_t)len);
      if (!fio_ct_is_eq(computed_mac, mac, 16)) {
        fio_secure_zero(computed_mac, sizeof(computed_mac));
        fio_secure_zero(comp_skey, sizeof(comp_skey));
        fio_secure_zero(sk_exp4, sizeof(sk_exp4));
        fio_secure_zero(&hk, sizeof(hk));
        fio_secure_zero(j0_enc, sizeof(j0_enc));
        return -1;
      }
    }

    ctr = 2;
    fio___aes_bs_gcm_ctr_process((uint8_t *)data,
                                 len,
                                 nonce_w,
                                 &ctr,
                                 10,
                                 sk_exp4,
                                 ghash_y,
                                 &hk,
                                 0);

    fio_secure_zero(sk_exp4, sizeof(sk_exp4));
    fio_secure_zero(&hk, sizeof(hk));
    fio_secure_zero(j0_enc, sizeof(j0_enc));
  }

  fio_secure_zero(comp_skey, sizeof(comp_skey));
  return 0;
}

SFUNC int fio_aes256_gcm_dec(void *restrict mac,
                             void *restrict data,
                             size_t len,
                             const void *ad,
                             size_t adlen,
                             const void *key,
                             const void *nonce) {
  uint64_t comp_skey[30];
  fio___aes_bs_key_expand256(comp_skey, (const uint8_t *)key);

  if (len <= 64 && adlen <= 16) {
    int ret = fio___aes_bs_gcm_dec_small(mac,
                                         (uint8_t *)data,
                                         len,
                                         ad,
                                         adlen,
                                         14,
                                         comp_skey,
                                         nonce);
    fio_secure_zero(comp_skey, sizeof(comp_skey));
    return ret;
  }

  {
    uint64_t sk_exp4[15 * 8];
    fio___ghash_ctmul_key_s hk;
    uint64_t ghash_y[2] = {0, 0};
    uint32_t nonce_w[3];
    uint8_t j0_enc[16];
    uint32_t ctr;

    fio___aes_bs_gcm_init(sk_exp4, &hk, j0_enc, nonce_w, 14, comp_skey, nonce);

    fio___aes_bs_ghash(ghash_y, &hk, (const uint8_t *)ad, adlen);
    fio___aes_bs_ghash(ghash_y, &hk, (const uint8_t *)data, len);

    {
      uint8_t computed_mac[16];
      fio___aes_bs_gcm_finalize(computed_mac,
                                ghash_y,
                                &hk,
                                j0_enc,
                                (uint64_t)adlen,
                                (uint64_t)len);
      if (!fio_ct_is_eq(computed_mac, mac, 16)) {
        fio_secure_zero(computed_mac, sizeof(computed_mac));
        fio_secure_zero(comp_skey, sizeof(comp_skey));
        fio_secure_zero(sk_exp4, sizeof(sk_exp4));
        fio_secure_zero(&hk, sizeof(hk));
        fio_secure_zero(j0_enc, sizeof(j0_enc));
        return -1;
      }
    }

    ctr = 2;
    fio___aes_bs_gcm_ctr_process((uint8_t *)data,
                                 len,
                                 nonce_w,
                                 &ctr,
                                 14,
                                 sk_exp4,
                                 ghash_y,
                                 &hk,
                                 0);

    fio_secure_zero(sk_exp4, sizeof(sk_exp4));
    fio_secure_zero(&hk, sizeof(hk));
    fio_secure_zero(j0_enc, sizeof(j0_enc));
  }

  fio_secure_zero(comp_skey, sizeof(comp_skey));
  return 0;
}

#endif /* Hardware vs Software implementation */

/* *****************************************************************************
Module Cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_AES
#endif /* FIO_AES */
