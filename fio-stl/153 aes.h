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

/* Precompute H powers for parallel GHASH: H, H², H³, H⁴ */
FIO_IFUNC void fio___x86_ghash_precompute(__m128i h, __m128i *htbl) {
  htbl[0] = h;                                         /* H */
  htbl[1] = fio___ghash_mult_pclmul(h, h);             /* H² */
  htbl[2] = fio___ghash_mult_pclmul(htbl[1], h);       /* H³ */
  htbl[3] = fio___ghash_mult_pclmul(htbl[1], htbl[1]); /* H⁴ */
}

/* 4-way parallel GHASH: compute (X0·H⁴) ^ (X1·H³) ^ (X2·H²) ^ (X3·H) */
FIO_IFUNC __m128i fio___x86_ghash_mult4(__m128i x0,
                                        __m128i x1,
                                        __m128i x2,
                                        __m128i x3,
                                        const __m128i *htbl) {
  __m128i r0 = fio___ghash_mult_pclmul(x0, htbl[3]); /* X0 · H⁴ */
  __m128i r1 = fio___ghash_mult_pclmul(x1, htbl[2]); /* X1 · H³ */
  __m128i r2 = fio___ghash_mult_pclmul(x2, htbl[1]); /* X2 · H² */
  __m128i r3 = fio___ghash_mult_pclmul(x3, htbl[0]); /* X3 · H  */
  return _mm_xor_si128(_mm_xor_si128(r0, r1), _mm_xor_si128(r2, r3));
}

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
  __m128i h, htbl[4], tag, ctr, j0;
  uint8_t *p = (uint8_t *)data;
  const uint8_t *aad = (const uint8_t *)ad;
  size_t orig_len = len;
  size_t orig_adlen = adlen;

  fio___aesni_gcm128_init(rk, &h, (const uint8_t *)key);
  h = fio___bswap128(h);

  /* Precompute H powers for parallel GHASH */
  fio___x86_ghash_precompute(h, htbl);

  uint8_t j0_bytes[16] = {0};
  FIO_MEMCPY(j0_bytes, nonce, 12);
  j0_bytes[15] = 1;
  j0 = _mm_loadu_si128((const __m128i *)j0_bytes);
  ctr = j0;
  tag = _mm_setzero_si128();

  /* GHASH over AAD - process 4 blocks at a time */
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

  /* Encrypt and GHASH - process 4 blocks at a time */
  while (len >= 64) {
    /* Generate 4 counters */
    __m128i ctr0 = fio___gcm_inc_ctr(ctr);
    __m128i ctr1 = fio___gcm_inc_ctr(ctr0);
    __m128i ctr2 = fio___gcm_inc_ctr(ctr1);
    __m128i ctr3 = fio___gcm_inc_ctr(ctr2);
    ctr = ctr3;

    /* Encrypt 4 blocks */
    __m128i ks0 = fio___aesni_encrypt128(ctr0, rk);
    __m128i ks1 = fio___aesni_encrypt128(ctr1, rk);
    __m128i ks2 = fio___aesni_encrypt128(ctr2, rk);
    __m128i ks3 = fio___aesni_encrypt128(ctr3, rk);

    /* Load plaintext and XOR with keystream */
    __m128i pt0 = _mm_loadu_si128((const __m128i *)p);
    __m128i pt1 = _mm_loadu_si128((const __m128i *)(p + 16));
    __m128i pt2 = _mm_loadu_si128((const __m128i *)(p + 32));
    __m128i pt3 = _mm_loadu_si128((const __m128i *)(p + 48));

    __m128i ct0 = _mm_xor_si128(pt0, ks0);
    __m128i ct1 = _mm_xor_si128(pt1, ks1);
    __m128i ct2 = _mm_xor_si128(pt2, ks2);
    __m128i ct3 = _mm_xor_si128(pt3, ks3);

    /* Store ciphertext */
    _mm_storeu_si128((__m128i *)p, ct0);
    _mm_storeu_si128((__m128i *)(p + 16), ct1);
    _mm_storeu_si128((__m128i *)(p + 32), ct2);
    _mm_storeu_si128((__m128i *)(p + 48), ct3);

    /* GHASH 4 blocks in parallel */
    ct0 = _mm_xor_si128(tag, fio___bswap128(ct0));
    tag = fio___x86_ghash_mult4(ct0,
                                fio___bswap128(ct1),
                                fio___bswap128(ct2),
                                fio___bswap128(ct3),
                                htbl);

    p += 64;
    len -= 64;
  }

  /* Handle remaining full blocks */
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
  __m128i h, htbl[4], tag, ctr, j0;
  uint8_t *p = (uint8_t *)data;
  const uint8_t *aad = (const uint8_t *)ad;
  size_t orig_len = len;
  size_t orig_adlen = adlen;

  fio___aesni_gcm256_init(rk, &h, (const uint8_t *)key);
  h = fio___bswap128(h);

  /* Precompute H powers for parallel GHASH */
  fio___x86_ghash_precompute(h, htbl);

  uint8_t j0_bytes[16] = {0};
  FIO_MEMCPY(j0_bytes, nonce, 12);
  j0_bytes[15] = 1;
  j0 = _mm_loadu_si128((const __m128i *)j0_bytes);
  ctr = j0;
  tag = _mm_setzero_si128();

  /* GHASH over AAD - process 4 blocks at a time */
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

  /* Encrypt and GHASH - process 4 blocks at a time */
  while (len >= 64) {
    /* Generate 4 counters */
    __m128i ctr0 = fio___gcm_inc_ctr(ctr);
    __m128i ctr1 = fio___gcm_inc_ctr(ctr0);
    __m128i ctr2 = fio___gcm_inc_ctr(ctr1);
    __m128i ctr3 = fio___gcm_inc_ctr(ctr2);
    ctr = ctr3;

    /* Encrypt 4 blocks */
    __m128i ks0 = fio___aesni_encrypt256(ctr0, rk);
    __m128i ks1 = fio___aesni_encrypt256(ctr1, rk);
    __m128i ks2 = fio___aesni_encrypt256(ctr2, rk);
    __m128i ks3 = fio___aesni_encrypt256(ctr3, rk);

    /* Load plaintext and XOR with keystream */
    __m128i pt0 = _mm_loadu_si128((const __m128i *)p);
    __m128i pt1 = _mm_loadu_si128((const __m128i *)(p + 16));
    __m128i pt2 = _mm_loadu_si128((const __m128i *)(p + 32));
    __m128i pt3 = _mm_loadu_si128((const __m128i *)(p + 48));

    __m128i ct0 = _mm_xor_si128(pt0, ks0);
    __m128i ct1 = _mm_xor_si128(pt1, ks1);
    __m128i ct2 = _mm_xor_si128(pt2, ks2);
    __m128i ct3 = _mm_xor_si128(pt3, ks3);

    /* Store ciphertext */
    _mm_storeu_si128((__m128i *)p, ct0);
    _mm_storeu_si128((__m128i *)(p + 16), ct1);
    _mm_storeu_si128((__m128i *)(p + 32), ct2);
    _mm_storeu_si128((__m128i *)(p + 48), ct3);

    /* GHASH 4 blocks in parallel */
    ct0 = _mm_xor_si128(tag, fio___bswap128(ct0));
    tag = fio___x86_ghash_mult4(ct0,
                                fio___bswap128(ct1),
                                fio___bswap128(ct2),
                                fio___bswap128(ct3),
                                htbl);

    p += 64;
    len -= 64;
  }

  /* Handle remaining full blocks */
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
  __m128i h, htbl[4], tag, ctr, j0;
  uint8_t *p = (uint8_t *)data;
  const uint8_t *aad = (const uint8_t *)ad;
  size_t orig_len = len;
  size_t orig_adlen = adlen;

  fio___aesni_gcm128_init(rk, &h, (const uint8_t *)key);
  h = fio___bswap128(h);

  /* Precompute H powers for parallel GHASH */
  fio___x86_ghash_precompute(h, htbl);

  uint8_t j0_bytes[16] = {0};
  FIO_MEMCPY(j0_bytes, nonce, 12);
  j0_bytes[15] = 1;
  j0 = _mm_loadu_si128((const __m128i *)j0_bytes);
  ctr = j0;
  tag = _mm_setzero_si128();

  /* GHASH over AAD - process 4 blocks at a time */
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

  /* GHASH over ciphertext - process 4 blocks at a time */
  const uint8_t *ct = p;
  size_t ct_len = orig_len;
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

  /* Decrypt - process 4 blocks at a time */
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

    __m128i c0 = _mm_loadu_si128((const __m128i *)p);
    __m128i c1 = _mm_loadu_si128((const __m128i *)(p + 16));
    __m128i c2 = _mm_loadu_si128((const __m128i *)(p + 32));
    __m128i c3 = _mm_loadu_si128((const __m128i *)(p + 48));

    _mm_storeu_si128((__m128i *)p, _mm_xor_si128(c0, ks0));
    _mm_storeu_si128((__m128i *)(p + 16), _mm_xor_si128(c1, ks1));
    _mm_storeu_si128((__m128i *)(p + 32), _mm_xor_si128(c2, ks2));
    _mm_storeu_si128((__m128i *)(p + 48), _mm_xor_si128(c3, ks3));

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
  __m128i h, htbl[4], tag, ctr, j0;
  uint8_t *p = (uint8_t *)data;
  const uint8_t *aad = (const uint8_t *)ad;
  size_t orig_len = len;
  size_t orig_adlen = adlen;

  fio___aesni_gcm256_init(rk, &h, (const uint8_t *)key);
  h = fio___bswap128(h);

  /* Precompute H powers for parallel GHASH */
  fio___x86_ghash_precompute(h, htbl);

  uint8_t j0_bytes[16] = {0};
  FIO_MEMCPY(j0_bytes, nonce, 12);
  j0_bytes[15] = 1;
  j0 = _mm_loadu_si128((const __m128i *)j0_bytes);
  ctr = j0;
  tag = _mm_setzero_si128();

  /* GHASH over AAD - process 4 blocks at a time */
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

  /* GHASH over ciphertext - process 4 blocks at a time */
  const uint8_t *ct = p;
  size_t ct_len = orig_len;
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

  /* Decrypt - process 4 blocks at a time */
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

    __m128i c0 = _mm_loadu_si128((const __m128i *)p);
    __m128i c1 = _mm_loadu_si128((const __m128i *)(p + 16));
    __m128i c2 = _mm_loadu_si128((const __m128i *)(p + 32));
    __m128i c3 = _mm_loadu_si128((const __m128i *)(p + 48));

    _mm_storeu_si128((__m128i *)p, _mm_xor_si128(c0, ks0));
    _mm_storeu_si128((__m128i *)(p + 16), _mm_xor_si128(c1, ks1));
    _mm_storeu_si128((__m128i *)(p + 32), _mm_xor_si128(c2, ks2));
    _mm_storeu_si128((__m128i *)(p + 48), _mm_xor_si128(c3, ks3));

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

/* AES S-box for key expansion SubWord */
static const uint8_t FIO___AES_SBOX_ARM[256] = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b,
    0xfe, 0xd7, 0xab, 0x76, 0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0,
    0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0, 0xb7, 0xfd, 0x93, 0x26,
    0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2,
    0xeb, 0x27, 0xb2, 0x75, 0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0,
    0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84, 0x53, 0xd1, 0x00, 0xed,
    0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f,
    0x50, 0x3c, 0x9f, 0xa8, 0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5,
    0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2, 0xcd, 0x0c, 0x13, 0xec,
    0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14,
    0xde, 0x5e, 0x0b, 0xdb, 0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c,
    0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79, 0xe7, 0xc8, 0x37, 0x6d,
    0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f,
    0x4b, 0xbd, 0x8b, 0x8a, 0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e,
    0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e, 0xe1, 0xf8, 0x98, 0x11,
    0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f,
    0xb0, 0x54, 0xbb, 0x16};

FIO_IFUNC uint32_t fio___arm_subword(uint32_t w) {
  return ((uint32_t)FIO___AES_SBOX_ARM[(w >> 0) & 0xFF] << 0) |
         ((uint32_t)FIO___AES_SBOX_ARM[(w >> 8) & 0xFF] << 8) |
         ((uint32_t)FIO___AES_SBOX_ARM[(w >> 16) & 0xFF] << 16) |
         ((uint32_t)FIO___AES_SBOX_ARM[(w >> 24) & 0xFF] << 24);
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
 * GCM bit ordering: byte[0] bit 7 = x^0, byte[15] bit 0 = x^127
 * GCM byte ordering: bytes 0-7 are HIGH (x^0 to x^63), bytes 8-15 are LOW (x^64
 * to x^127)
 *
 * After vrbitq_u8:
 * - byte[0] bit 0 = x^0, byte[7] bit 7 = x^63 (lane 0 = x^0 to x^63)
 * - byte[8] bit 0 = x^64, byte[15] bit 7 = x^127 (lane 1 = x^64 to x^127)
 *
 * So lane 0 contains LOW powers (x^0 to x^63) and lane 1 contains HIGH powers
 * (x^64 to x^127). This is the OPPOSITE of what we want for standard polynomial
 * multiplication!
 */
FIO_IFUNC uint8x16_t fio___arm_ghash_mult_pmull(uint8x16_t x_vec,
                                                uint8x16_t h_vec) {
  /* Reverse bits within each byte */
  uint8x16_t a = vrbitq_u8(x_vec);
  uint8x16_t b = vrbitq_u8(h_vec);

  /* lane 0 = x^0 to x^63 (low), lane 1 = x^64 to x^127 (high)
   * For polynomial (a_hi*x^64 + a_lo), we have:
   * a_lo = lane 0 (coefficients of x^0 to x^63)
   * a_hi = lane 1 (coefficients of x^64 to x^127) */
  poly64_t a_lo = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(a), 0);
  poly64_t a_hi = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(a), 1);
  poly64_t b_lo = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(b), 0);
  poly64_t b_hi = (poly64_t)vgetq_lane_u64(vreinterpretq_u64_u8(b), 1);

  /* Karatsuba: (a_hi*x^64 + a_lo) * (b_hi*x^64 + b_lo) */
  poly128_t p_ll = vmull_p64(a_lo, b_lo); /* x^0 to x^126 */
  poly128_t p_hh = vmull_p64(a_hi, b_hi); /* x^128 to x^254 */
  poly128_t p_mid = vmull_p64((poly64_t)(a_lo ^ a_hi), (poly64_t)(b_lo ^ b_hi));

  uint64x2_t ll = vreinterpretq_u64_p128(p_ll);
  uint64x2_t hh = vreinterpretq_u64_p128(p_hh);
  uint64x2_t mm = vreinterpretq_u64_p128(p_mid);
  mm = veorq_u64(mm, ll);
  mm = veorq_u64(mm, hh);

  /* 256-bit product: r3:r2:r1:r0 where r0 is bits 0-63, r3 is bits 192-255 */
  uint64_t r0 = vgetq_lane_u64(ll, 0);
  uint64_t r1 = vgetq_lane_u64(ll, 1) ^ vgetq_lane_u64(mm, 0);
  uint64_t r2 = vgetq_lane_u64(hh, 0) ^ vgetq_lane_u64(mm, 1);
  uint64_t r3 = vgetq_lane_u64(hh, 1);

  /* Reduce modulo x^128 + x^7 + x^2 + x + 1
   * x^128 ≡ x^7 + x^2 + x + 1
   *
   * For a bit at position 128+k (in r2 or r3), we need to XOR into position k
   * plus the polynomial terms at k+7, k+2, k+1.
   *
   * r2 contains bits 128-191 (x^128 to x^191)
   * r3 contains bits 192-255 (x^192 to x^255)
   *
   * Bit k of r2 represents x^(128+k), which reduces to x^k + x^(k+7) + x^(k+2)
   * + x^(k+1) These go into r0 (for k < 64) and r1 (for k+7 >= 64, etc.) */

  /* Fold r3 (bits 192-255) into r2:r1:r0
   * Bit k of r3 = x^(192+k) ≡ x^(64+k) + x^(71+k) + x^(66+k) + x^(65+k)
   * x^(64+k) goes to r1 bit k
   * x^(71+k) goes to r1 bit (k+7) if k+7 < 64, else r2 bit (k+7-64)
   * etc. */
  r1 ^= r3;         /* x^(64+k) */
  r1 ^= (r3 << 7);  /* x^(71+k) for k < 57 */
  r2 ^= (r3 >> 57); /* x^(71+k) for k >= 57, wraps to r2 */
  r1 ^= (r3 << 2);  /* x^(66+k) for k < 62 */
  r2 ^= (r3 >> 62); /* x^(66+k) for k >= 62 */
  r1 ^= (r3 << 1);  /* x^(65+k) for k < 63 */
  r2 ^= (r3 >> 63); /* x^(65+k) for k = 63 */

  /* Fold r2 (bits 128-191) into r1:r0
   * Bit k of r2 = x^(128+k) ≡ x^k + x^(k+7) + x^(k+2) + x^(k+1)
   * x^k goes to r0 bit k
   * x^(k+7) goes to r0 bit (k+7) if k+7 < 64, else r1 bit (k+7-64)
   * etc. */
  r0 ^= r2;         /* x^k */
  r0 ^= (r2 << 7);  /* x^(k+7) for k < 57 */
  r1 ^= (r2 >> 57); /* x^(k+7) for k >= 57 */
  r0 ^= (r2 << 2);  /* x^(k+2) for k < 62 */
  r1 ^= (r2 >> 62); /* x^(k+2) for k >= 62 */
  r0 ^= (r2 << 1);  /* x^(k+1) for k < 63 */
  r1 ^= (r2 >> 63); /* x^(k+1) for k = 63 */

  /* Result is r1:r0 where r0 = x^0 to x^63, r1 = x^64 to x^127
   * This maps to: lane 0 = r0, lane 1 = r1
   * Then vrbitq_u8 converts back to GCM format */
  uint64x2_t result = vcombine_u64(vcreate_u64(r0), vcreate_u64(r1));
  return vrbitq_u8(vreinterpretq_u8_u64(result));
}

/* Wrapper for the PMULL implementation */
FIO_IFUNC uint8x16_t fio___arm_ghash_mult(uint8x16_t x_vec, uint8x16_t h_vec) {
  return fio___arm_ghash_mult_pmull(x_vec, h_vec);
}

/* Precompute H powers for parallel GHASH: H, H², H³, H⁴ */
FIO_IFUNC void fio___arm_ghash_precompute(uint8x16_t h, uint8x16_t *htbl) {
  htbl[0] = h;                                      /* H */
  htbl[1] = fio___arm_ghash_mult(h, h);             /* H² */
  htbl[2] = fio___arm_ghash_mult(htbl[1], h);       /* H³ */
  htbl[3] = fio___arm_ghash_mult(htbl[1], htbl[1]); /* H⁴ */
}

/* 4-way parallel GHASH: compute (X0·H⁴) ^ (X1·H³) ^ (X2·H²) ^ (X3·H)
 * This processes 4 blocks at once using precomputed H powers */
FIO_IFUNC uint8x16_t fio___arm_ghash_mult4(uint8x16_t x0,
                                           uint8x16_t x1,
                                           uint8x16_t x2,
                                           uint8x16_t x3,
                                           const uint8x16_t *htbl) {
  /* Compute all 4 products and XOR them together */
  uint8x16_t r0 = fio___arm_ghash_mult(x0, htbl[3]); /* X0 · H⁴ */
  uint8x16_t r1 = fio___arm_ghash_mult(x1, htbl[2]); /* X1 · H³ */
  uint8x16_t r2 = fio___arm_ghash_mult(x2, htbl[1]); /* X2 · H² */
  uint8x16_t r3 = fio___arm_ghash_mult(x3, htbl[0]); /* X3 · H  */
  return veorq_u8(veorq_u8(r0, r1), veorq_u8(r2, r3));
}

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
  c = __builtin_bswap32(__builtin_bswap32(c) + 1);
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
  uint8x16_t h, htbl[4], tag, ctr, j0;
  uint8_t *p = (uint8_t *)data;
  const uint8_t *aad = (const uint8_t *)ad;
  size_t orig_len = len;
  size_t orig_adlen = adlen;

  fio___arm_aes128_key_expand(rk, (const uint8_t *)key);
  uint8x16_t zero = vdupq_n_u8(0);
  h = fio___arm_aes128_encrypt(zero, rk);

  /* Precompute H powers for parallel GHASH */
  fio___arm_ghash_precompute(h, htbl);

  uint8_t j0_bytes[16] = {0};
  FIO_MEMCPY(j0_bytes, nonce, 12);
  j0_bytes[15] = 1;
  j0 = vld1q_u8(j0_bytes);
  ctr = j0;
  tag = vdupq_n_u8(0);

  /* GHASH over AAD - process 4 blocks at a time */
  while (adlen >= 64) {
    uint8x16_t a0 = vld1q_u8(aad);
    uint8x16_t a1 = vld1q_u8(aad + 16);
    uint8x16_t a2 = vld1q_u8(aad + 32);
    uint8x16_t a3 = vld1q_u8(aad + 48);
    a0 = veorq_u8(tag, a0);
    tag = fio___arm_ghash_mult4(a0, a1, a2, a3, htbl);
    aad += 64;
    adlen -= 64;
  }
  while (adlen >= 16) {
    uint8x16_t aad_block = vld1q_u8(aad);
    tag = veorq_u8(tag, aad_block);
    tag = fio___arm_ghash_mult(tag, h);
    aad += 16;
    adlen -= 16;
  }
  if (adlen > 0) {
    uint8_t tmp[16] = {0};
    FIO_MEMCPY(tmp, aad, adlen);
    uint8x16_t aad_block = vld1q_u8(tmp);
    tag = veorq_u8(tag, aad_block);
    tag = fio___arm_ghash_mult(tag, h);
  }

  /* Encrypt and GHASH - process 4 blocks at a time with interleaving */
  while (len >= 64) {
    /* Generate 4 counters */
    uint8x16_t ctr0 = fio___arm_gcm_inc_ctr(ctr);
    uint8x16_t ctr1 = fio___arm_gcm_inc_ctr(ctr0);
    uint8x16_t ctr2 = fio___arm_gcm_inc_ctr(ctr1);
    uint8x16_t ctr3 = fio___arm_gcm_inc_ctr(ctr2);
    ctr = ctr3;

    /* Encrypt 4 blocks (AES operations can be pipelined by CPU) */
    uint8x16_t ks0 = fio___arm_aes128_encrypt(ctr0, rk);
    uint8x16_t ks1 = fio___arm_aes128_encrypt(ctr1, rk);
    uint8x16_t ks2 = fio___arm_aes128_encrypt(ctr2, rk);
    uint8x16_t ks3 = fio___arm_aes128_encrypt(ctr3, rk);

    /* Load plaintext and XOR with keystream */
    uint8x16_t pt0 = vld1q_u8(p);
    uint8x16_t pt1 = vld1q_u8(p + 16);
    uint8x16_t pt2 = vld1q_u8(p + 32);
    uint8x16_t pt3 = vld1q_u8(p + 48);

    uint8x16_t ct0 = veorq_u8(pt0, ks0);
    uint8x16_t ct1 = veorq_u8(pt1, ks1);
    uint8x16_t ct2 = veorq_u8(pt2, ks2);
    uint8x16_t ct3 = veorq_u8(pt3, ks3);

    /* Store ciphertext */
    vst1q_u8(p, ct0);
    vst1q_u8(p + 16, ct1);
    vst1q_u8(p + 32, ct2);
    vst1q_u8(p + 48, ct3);

    /* GHASH 4 blocks in parallel */
    ct0 = veorq_u8(tag, ct0);
    tag = fio___arm_ghash_mult4(ct0, ct1, ct2, ct3, htbl);

    p += 64;
    len -= 64;
  }

  /* Handle remaining full blocks */
  while (len >= 16) {
    ctr = fio___arm_gcm_inc_ctr(ctr);
    uint8x16_t keystream = fio___arm_aes128_encrypt(ctr, rk);
    uint8x16_t plaintext = vld1q_u8(p);
    uint8x16_t ciphertext = veorq_u8(plaintext, keystream);
    vst1q_u8(p, ciphertext);
    tag = veorq_u8(tag, ciphertext);
    tag = fio___arm_ghash_mult(tag, h);
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
    uint8x16_t ct_block = vld1q_u8(tmp);
    tag = veorq_u8(tag, ct_block);
    tag = fio___arm_ghash_mult(tag, h);
  }

  /* GHASH length block */
  uint8_t len_block[16] = {0};
  fio_u2buf64_be(len_block, (uint64_t)orig_adlen * 8);
  fio_u2buf64_be(len_block + 8, (uint64_t)orig_len * 8);
  uint8x16_t len_blk = vld1q_u8(len_block);
  tag = veorq_u8(tag, len_blk);
  tag = fio___arm_ghash_mult(tag, h);

  /* Final tag */
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
  uint8x16_t h, htbl[4], tag, ctr, j0;
  uint8_t *p = (uint8_t *)data;
  const uint8_t *aad = (const uint8_t *)ad;
  size_t orig_len = len;
  size_t orig_adlen = adlen;

  fio___arm_aes256_key_expand(rk, (const uint8_t *)key);
  uint8x16_t zero = vdupq_n_u8(0);
  h = fio___arm_aes256_encrypt(zero, rk);

  /* Precompute H powers for parallel GHASH */
  fio___arm_ghash_precompute(h, htbl);

  uint8_t j0_bytes[16] = {0};
  FIO_MEMCPY(j0_bytes, nonce, 12);
  j0_bytes[15] = 1;
  j0 = vld1q_u8(j0_bytes);
  ctr = j0;
  tag = vdupq_n_u8(0);

  /* GHASH over AAD - process 4 blocks at a time */
  while (adlen >= 64) {
    uint8x16_t a0 = vld1q_u8(aad);
    uint8x16_t a1 = vld1q_u8(aad + 16);
    uint8x16_t a2 = vld1q_u8(aad + 32);
    uint8x16_t a3 = vld1q_u8(aad + 48);
    a0 = veorq_u8(tag, a0);
    tag = fio___arm_ghash_mult4(a0, a1, a2, a3, htbl);
    aad += 64;
    adlen -= 64;
  }
  while (adlen >= 16) {
    uint8x16_t aad_block = vld1q_u8(aad);
    tag = veorq_u8(tag, aad_block);
    tag = fio___arm_ghash_mult(tag, h);
    aad += 16;
    adlen -= 16;
  }
  if (adlen > 0) {
    uint8_t tmp[16] = {0};
    FIO_MEMCPY(tmp, aad, adlen);
    uint8x16_t aad_block = vld1q_u8(tmp);
    tag = veorq_u8(tag, aad_block);
    tag = fio___arm_ghash_mult(tag, h);
  }

  /* Encrypt and GHASH - process 4 blocks at a time with interleaving */
  while (len >= 64) {
    /* Generate 4 counters */
    uint8x16_t ctr0 = fio___arm_gcm_inc_ctr(ctr);
    uint8x16_t ctr1 = fio___arm_gcm_inc_ctr(ctr0);
    uint8x16_t ctr2 = fio___arm_gcm_inc_ctr(ctr1);
    uint8x16_t ctr3 = fio___arm_gcm_inc_ctr(ctr2);
    ctr = ctr3;

    /* Encrypt 4 blocks (AES operations can be pipelined by CPU) */
    uint8x16_t ks0 = fio___arm_aes256_encrypt(ctr0, rk);
    uint8x16_t ks1 = fio___arm_aes256_encrypt(ctr1, rk);
    uint8x16_t ks2 = fio___arm_aes256_encrypt(ctr2, rk);
    uint8x16_t ks3 = fio___arm_aes256_encrypt(ctr3, rk);

    /* Load plaintext and XOR with keystream */
    uint8x16_t pt0 = vld1q_u8(p);
    uint8x16_t pt1 = vld1q_u8(p + 16);
    uint8x16_t pt2 = vld1q_u8(p + 32);
    uint8x16_t pt3 = vld1q_u8(p + 48);

    uint8x16_t ct0 = veorq_u8(pt0, ks0);
    uint8x16_t ct1 = veorq_u8(pt1, ks1);
    uint8x16_t ct2 = veorq_u8(pt2, ks2);
    uint8x16_t ct3 = veorq_u8(pt3, ks3);

    /* Store ciphertext */
    vst1q_u8(p, ct0);
    vst1q_u8(p + 16, ct1);
    vst1q_u8(p + 32, ct2);
    vst1q_u8(p + 48, ct3);

    /* GHASH 4 blocks in parallel */
    ct0 = veorq_u8(tag, ct0);
    tag = fio___arm_ghash_mult4(ct0, ct1, ct2, ct3, htbl);

    p += 64;
    len -= 64;
  }

  /* Handle remaining full blocks */
  while (len >= 16) {
    ctr = fio___arm_gcm_inc_ctr(ctr);
    uint8x16_t keystream = fio___arm_aes256_encrypt(ctr, rk);
    uint8x16_t plaintext = vld1q_u8(p);
    uint8x16_t ciphertext = veorq_u8(plaintext, keystream);
    vst1q_u8(p, ciphertext);
    tag = veorq_u8(tag, ciphertext);
    tag = fio___arm_ghash_mult(tag, h);
    p += 16;
    len -= 16;
  }

  /* Handle partial final block */
  if (len > 0) {
    ctr = fio___arm_gcm_inc_ctr(ctr);
    uint8x16_t keystream = fio___arm_aes256_encrypt(ctr, rk);
    uint8_t ks_bytes[16];
    vst1q_u8(ks_bytes, keystream);
    for (size_t i = 0; i < len; ++i)
      p[i] ^= ks_bytes[i];
    uint8_t tmp[16] = {0};
    FIO_MEMCPY(tmp, p, len);
    uint8x16_t ct_block = vld1q_u8(tmp);
    tag = veorq_u8(tag, ct_block);
    tag = fio___arm_ghash_mult(tag, h);
  }

  /* GHASH length block */
  uint8_t len_block[16] = {0};
  fio_u2buf64_be(len_block, (uint64_t)orig_adlen * 8);
  fio_u2buf64_be(len_block + 8, (uint64_t)orig_len * 8);
  uint8x16_t len_blk = vld1q_u8(len_block);
  tag = veorq_u8(tag, len_blk);
  tag = fio___arm_ghash_mult(tag, h);

  /* Final tag */
  uint8x16_t s = fio___arm_aes256_encrypt(j0, rk);
  tag = veorq_u8(tag, s);
  vst1q_u8((uint8_t *)mac, tag);

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
  uint8x16_t rk[11];
  uint8x16_t h, htbl[4], tag, ctr, j0;
  uint8_t *p = (uint8_t *)data;
  const uint8_t *aad = (const uint8_t *)ad;
  size_t orig_len = len;
  size_t orig_adlen = adlen;

  fio___arm_aes128_key_expand(rk, (const uint8_t *)key);
  uint8x16_t zero = vdupq_n_u8(0);
  h = fio___arm_aes128_encrypt(zero, rk);

  /* Precompute H powers for parallel GHASH */
  fio___arm_ghash_precompute(h, htbl);

  uint8_t j0_bytes[16] = {0};
  FIO_MEMCPY(j0_bytes, nonce, 12);
  j0_bytes[15] = 1;
  j0 = vld1q_u8(j0_bytes);
  ctr = j0;
  tag = vdupq_n_u8(0);

  /* GHASH over AAD - process 4 blocks at a time */
  while (adlen >= 64) {
    uint8x16_t a0 = vld1q_u8(aad);
    uint8x16_t a1 = vld1q_u8(aad + 16);
    uint8x16_t a2 = vld1q_u8(aad + 32);
    uint8x16_t a3 = vld1q_u8(aad + 48);
    a0 = veorq_u8(tag, a0);
    tag = fio___arm_ghash_mult4(a0, a1, a2, a3, htbl);
    aad += 64;
    adlen -= 64;
  }
  while (adlen >= 16) {
    uint8x16_t aad_block = vld1q_u8(aad);
    tag = veorq_u8(tag, aad_block);
    tag = fio___arm_ghash_mult(tag, h);
    aad += 16;
    adlen -= 16;
  }
  if (adlen > 0) {
    uint8_t tmp[16] = {0};
    FIO_MEMCPY(tmp, aad, adlen);
    uint8x16_t aad_block = vld1q_u8(tmp);
    tag = veorq_u8(tag, aad_block);
    tag = fio___arm_ghash_mult(tag, h);
  }

  /* GHASH over ciphertext - process 4 blocks at a time */
  const uint8_t *ct = p;
  size_t ct_len = orig_len;
  while (ct_len >= 64) {
    uint8x16_t c0 = vld1q_u8(ct);
    uint8x16_t c1 = vld1q_u8(ct + 16);
    uint8x16_t c2 = vld1q_u8(ct + 32);
    uint8x16_t c3 = vld1q_u8(ct + 48);
    c0 = veorq_u8(tag, c0);
    tag = fio___arm_ghash_mult4(c0, c1, c2, c3, htbl);
    ct += 64;
    ct_len -= 64;
  }
  while (ct_len >= 16) {
    uint8x16_t ct_block = vld1q_u8(ct);
    tag = veorq_u8(tag, ct_block);
    tag = fio___arm_ghash_mult(tag, h);
    ct += 16;
    ct_len -= 16;
  }
  if (ct_len > 0) {
    uint8_t tmp[16] = {0};
    FIO_MEMCPY(tmp, ct, ct_len);
    uint8x16_t ct_block = vld1q_u8(tmp);
    tag = veorq_u8(tag, ct_block);
    tag = fio___arm_ghash_mult(tag, h);
  }

  /* GHASH length block */
  uint8_t len_block[16] = {0};
  fio_u2buf64_be(len_block, (uint64_t)orig_adlen * 8);
  fio_u2buf64_be(len_block + 8, (uint64_t)orig_len * 8);
  uint8x16_t len_blk = vld1q_u8(len_block);
  tag = veorq_u8(tag, len_blk);
  tag = fio___arm_ghash_mult(tag, h);

  /* Compute and verify tag */
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

  /* Decrypt - process 4 blocks at a time */
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

    uint8x16_t c0 = vld1q_u8(p);
    uint8x16_t c1 = vld1q_u8(p + 16);
    uint8x16_t c2 = vld1q_u8(p + 32);
    uint8x16_t c3 = vld1q_u8(p + 48);

    vst1q_u8(p, veorq_u8(c0, ks0));
    vst1q_u8(p + 16, veorq_u8(c1, ks1));
    vst1q_u8(p + 32, veorq_u8(c2, ks2));
    vst1q_u8(p + 48, veorq_u8(c3, ks3));

    p += 64;
    len -= 64;
  }
  while (len >= 16) {
    ctr = fio___arm_gcm_inc_ctr(ctr);
    uint8x16_t keystream = fio___arm_aes128_encrypt(ctr, rk);
    uint8x16_t ciphertext = vld1q_u8(p);
    uint8x16_t plaintext = veorq_u8(ciphertext, keystream);
    vst1q_u8(p, plaintext);
    p += 16;
    len -= 16;
  }
  if (len > 0) {
    ctr = fio___arm_gcm_inc_ctr(ctr);
    uint8x16_t keystream = fio___arm_aes128_encrypt(ctr, rk);
    uint8_t ks_bytes[16];
    vst1q_u8(ks_bytes, keystream);
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
  uint8x16_t rk[15];
  uint8x16_t h, htbl[4], tag, ctr, j0;
  uint8_t *p = (uint8_t *)data;
  const uint8_t *aad = (const uint8_t *)ad;
  size_t orig_len = len;
  size_t orig_adlen = adlen;

  fio___arm_aes256_key_expand(rk, (const uint8_t *)key);
  uint8x16_t zero = vdupq_n_u8(0);
  h = fio___arm_aes256_encrypt(zero, rk);

  /* Precompute H powers for parallel GHASH */
  fio___arm_ghash_precompute(h, htbl);

  uint8_t j0_bytes[16] = {0};
  FIO_MEMCPY(j0_bytes, nonce, 12);
  j0_bytes[15] = 1;
  j0 = vld1q_u8(j0_bytes);
  ctr = j0;
  tag = vdupq_n_u8(0);

  /* GHASH over AAD - process 4 blocks at a time */
  while (adlen >= 64) {
    uint8x16_t a0 = vld1q_u8(aad);
    uint8x16_t a1 = vld1q_u8(aad + 16);
    uint8x16_t a2 = vld1q_u8(aad + 32);
    uint8x16_t a3 = vld1q_u8(aad + 48);
    a0 = veorq_u8(tag, a0);
    tag = fio___arm_ghash_mult4(a0, a1, a2, a3, htbl);
    aad += 64;
    adlen -= 64;
  }
  while (adlen >= 16) {
    uint8x16_t aad_block = vld1q_u8(aad);
    tag = veorq_u8(tag, aad_block);
    tag = fio___arm_ghash_mult(tag, h);
    aad += 16;
    adlen -= 16;
  }
  if (adlen > 0) {
    uint8_t tmp[16] = {0};
    FIO_MEMCPY(tmp, aad, adlen);
    uint8x16_t aad_block = vld1q_u8(tmp);
    tag = veorq_u8(tag, aad_block);
    tag = fio___arm_ghash_mult(tag, h);
  }

  /* GHASH over ciphertext - process 4 blocks at a time */
  const uint8_t *ct = p;
  size_t ct_len = orig_len;
  while (ct_len >= 64) {
    uint8x16_t c0 = vld1q_u8(ct);
    uint8x16_t c1 = vld1q_u8(ct + 16);
    uint8x16_t c2 = vld1q_u8(ct + 32);
    uint8x16_t c3 = vld1q_u8(ct + 48);
    c0 = veorq_u8(tag, c0);
    tag = fio___arm_ghash_mult4(c0, c1, c2, c3, htbl);
    ct += 64;
    ct_len -= 64;
  }
  while (ct_len >= 16) {
    uint8x16_t ct_block = vld1q_u8(ct);
    tag = veorq_u8(tag, ct_block);
    tag = fio___arm_ghash_mult(tag, h);
    ct += 16;
    ct_len -= 16;
  }
  if (ct_len > 0) {
    uint8_t tmp[16] = {0};
    FIO_MEMCPY(tmp, ct, ct_len);
    uint8x16_t ct_block = vld1q_u8(tmp);
    tag = veorq_u8(tag, ct_block);
    tag = fio___arm_ghash_mult(tag, h);
  }

  /* GHASH length block */
  uint8_t len_block[16] = {0};
  fio_u2buf64_be(len_block, (uint64_t)orig_adlen * 8);
  fio_u2buf64_be(len_block + 8, (uint64_t)orig_len * 8);
  uint8x16_t len_blk = vld1q_u8(len_block);
  tag = veorq_u8(tag, len_blk);
  tag = fio___arm_ghash_mult(tag, h);

  /* Compute and verify tag */
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

  /* Decrypt - process 4 blocks at a time */
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

    uint8x16_t c0 = vld1q_u8(p);
    uint8x16_t c1 = vld1q_u8(p + 16);
    uint8x16_t c2 = vld1q_u8(p + 32);
    uint8x16_t c3 = vld1q_u8(p + 48);

    vst1q_u8(p, veorq_u8(c0, ks0));
    vst1q_u8(p + 16, veorq_u8(c1, ks1));
    vst1q_u8(p + 32, veorq_u8(c2, ks2));
    vst1q_u8(p + 48, veorq_u8(c3, ks3));

    p += 64;
    len -= 64;
  }
  while (len >= 16) {
    ctr = fio___arm_gcm_inc_ctr(ctr);
    uint8x16_t keystream = fio___arm_aes256_encrypt(ctr, rk);
    uint8x16_t ciphertext = vld1q_u8(p);
    uint8x16_t plaintext = veorq_u8(ciphertext, keystream);
    vst1q_u8(p, plaintext);
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
  }
  /* Clear sensitive data */
  fio_secure_zero(rk, sizeof(rk));
  fio_secure_zero(htbl, sizeof(htbl));
  fio_secure_zero(j0_bytes, sizeof(j0_bytes));
  return 0;
}

/* *****************************************************************************
Portable (Software) Implementation - Fallback
***************************************************************************** */
#else /* No hardware acceleration */

/* clang-format off */
/* AES forward S-box */
static const uint8_t FIO___AES_SBOX[256] = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5,
    0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0,
    0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc,
    0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a,
    0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0,
    0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b,
    0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85,
    0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5,
    0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17,
    0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88,
    0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c,
    0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9,
    0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6,
    0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e,
    0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94,
    0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68,
    0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
};

/* AES round constants */
static const uint8_t FIO___AES_RCON[11] = {
    0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36
};

/* Pre-computed T-tables for AES encryption */
static const uint32_t FIO___AES_TE0[256] = {
    0xc66363a5, 0xf87c7c84, 0xee777799, 0xf67b7b8d, 0xfff2f20d, 0xd66b6bbd, 0xde6f6fb1, 0x91c5c554,
    0x60303050, 0x02010103, 0xce6767a9, 0x562b2b7d, 0xe7fefe19, 0xb5d7d762, 0x4dababe6, 0xec76769a,
    0x8fcaca45, 0x1f82829d, 0x89c9c940, 0xfa7d7d87, 0xeffafa15, 0xb25959eb, 0x8e4747c9, 0xfbf0f00b,
    0x41adadec, 0xb3d4d467, 0x5fa2a2fd, 0x45afafea, 0x239c9cbf, 0x53a4a4f7, 0xe4727296, 0x9bc0c05b,
    0x75b7b7c2, 0xe1fdfd1c, 0x3d9393ae, 0x4c26266a, 0x6c36365a, 0x7e3f3f41, 0xf5f7f702, 0x83cccc4f,
    0x6834345c, 0x51a5a5f4, 0xd1e5e534, 0xf9f1f108, 0xe2717193, 0xabd8d873, 0x62313153, 0x2a15153f,
    0x0804040c, 0x95c7c752, 0x46232365, 0x9dc3c35e, 0x30181828, 0x379696a1, 0x0a05050f, 0x2f9a9ab5,
    0x0e070709, 0x24121236, 0x1b80809b, 0xdfe2e23d, 0xcdebeb26, 0x4e272769, 0x7fb2b2cd, 0xea75759f,
    0x1209091b, 0x1d83839e, 0x582c2c74, 0x341a1a2e, 0x361b1b2d, 0xdc6e6eb2, 0xb45a5aee, 0x5ba0a0fb,
    0xa45252f6, 0x763b3b4d, 0xb7d6d661, 0x7db3b3ce, 0x5229297b, 0xdde3e33e, 0x5e2f2f71, 0x13848497,
    0xa65353f5, 0xb9d1d168, 0x00000000, 0xc1eded2c, 0x40202060, 0xe3fcfc1f, 0x79b1b1c8, 0xb65b5bed,
    0xd46a6abe, 0x8dcbcb46, 0x67bebed9, 0x7239394b, 0x944a4ade, 0x984c4cd4, 0xb05858e8, 0x85cfcf4a,
    0xbbd0d06b, 0xc5efef2a, 0x4faaaae5, 0xedfbfb16, 0x864343c5, 0x9a4d4dd7, 0x66333355, 0x11858594,
    0x8a4545cf, 0xe9f9f910, 0x04020206, 0xfe7f7f81, 0xa05050f0, 0x783c3c44, 0x259f9fba, 0x4ba8a8e3,
    0xa25151f3, 0x5da3a3fe, 0x804040c0, 0x058f8f8a, 0x3f9292ad, 0x219d9dbc, 0x70383848, 0xf1f5f504,
    0x63bcbcdf, 0x77b6b6c1, 0xafdada75, 0x42212163, 0x20101030, 0xe5ffff1a, 0xfdf3f30e, 0xbfd2d26d,
    0x81cdcd4c, 0x180c0c14, 0x26131335, 0xc3ecec2f, 0xbe5f5fe1, 0x359797a2, 0x884444cc, 0x2e171739,
    0x93c4c457, 0x55a7a7f2, 0xfc7e7e82, 0x7a3d3d47, 0xc86464ac, 0xba5d5de7, 0x3219192b, 0xe6737395,
    0xc06060a0, 0x19818198, 0x9e4f4fd1, 0xa3dcdc7f, 0x44222266, 0x542a2a7e, 0x3b9090ab, 0x0b888883,
    0x8c4646ca, 0xc7eeee29, 0x6bb8b8d3, 0x2814143c, 0xa7dede79, 0xbc5e5ee2, 0x160b0b1d, 0xaddbdb76,
    0xdbe0e03b, 0x64323256, 0x743a3a4e, 0x140a0a1e, 0x924949db, 0x0c06060a, 0x4824246c, 0xb85c5ce4,
    0x9fc2c25d, 0xbdd3d36e, 0x43acacef, 0xc46262a6, 0x399191a8, 0x319595a4, 0xd3e4e437, 0xf279798b,
    0xd5e7e732, 0x8bc8c843, 0x6e373759, 0xda6d6db7, 0x018d8d8c, 0xb1d5d564, 0x9c4e4ed2, 0x49a9a9e0,
    0xd86c6cb4, 0xac5656fa, 0xf3f4f407, 0xcfeaea25, 0xca6565af, 0xf47a7a8e, 0x47aeaee9, 0x10080818,
    0x6fbabad5, 0xf0787888, 0x4a25256f, 0x5c2e2e72, 0x381c1c24, 0x57a6a6f1, 0x73b4b4c7, 0x97c6c651,
    0xcbe8e823, 0xa1dddd7c, 0xe874749c, 0x3e1f1f21, 0x964b4bdd, 0x61bdbddc, 0x0d8b8b86, 0x0f8a8a85,
    0xe0707090, 0x7c3e3e42, 0x71b5b5c4, 0xcc6666aa, 0x904848d8, 0x06030305, 0xf7f6f601, 0x1c0e0e12,
    0xc26161a3, 0x6a35355f, 0xae5757f9, 0x69b9b9d0, 0x17868691, 0x99c1c158, 0x3a1d1d27, 0x279e9eb9,
    0xd9e1e138, 0xebf8f813, 0x2b9898b3, 0x22111133, 0xd26969bb, 0xa9d9d970, 0x078e8e89, 0x339494a7,
    0x2d9b9bb6, 0x3c1e1e22, 0x15878792, 0xc9e9e920, 0x87cece49, 0xaa5555ff, 0x50282878, 0xa5dfdf7a,
    0x038c8c8f, 0x59a1a1f8, 0x09898980, 0x1a0d0d17, 0x65bfbfda, 0xd7e6e631, 0x844242c6, 0xd06868b8,
    0x824141c3, 0x299999b0, 0x5a2d2d77, 0x1e0f0f11, 0x7bb0b0cb, 0xa85454fc, 0x6dbbbbd6, 0x2c16163a
};

static const uint32_t FIO___AES_TE1[256] = {
    0xa5c66363, 0x84f87c7c, 0x99ee7777, 0x8df67b7b, 0x0dfff2f2, 0xbdd66b6b, 0xb1de6f6f, 0x5491c5c5,
    0x50603030, 0x03020101, 0xa9ce6767, 0x7d562b2b, 0x19e7fefe, 0x62b5d7d7, 0xe64dabab, 0x9aec7676,
    0x458fcaca, 0x9d1f8282, 0x4089c9c9, 0x87fa7d7d, 0x15effafa, 0xebb25959, 0xc98e4747, 0x0bfbf0f0,
    0xec41adad, 0x67b3d4d4, 0xfd5fa2a2, 0xea45afaf, 0xbf239c9c, 0xf753a4a4, 0x96e47272, 0x5b9bc0c0,
    0xc275b7b7, 0x1ce1fdfd, 0xae3d9393, 0x6a4c2626, 0x5a6c3636, 0x417e3f3f, 0x02f5f7f7, 0x4f83cccc,
    0x5c683434, 0xf451a5a5, 0x34d1e5e5, 0x08f9f1f1, 0x93e27171, 0x73abd8d8, 0x53623131, 0x3f2a1515,
    0x0c080404, 0x5295c7c7, 0x65462323, 0x5e9dc3c3, 0x28301818, 0xa1379696, 0x0f0a0505, 0xb52f9a9a,
    0x090e0707, 0x36241212, 0x9b1b8080, 0x3ddfe2e2, 0x26cdebeb, 0x694e2727, 0xcd7fb2b2, 0x9fea7575,
    0x1b120909, 0x9e1d8383, 0x74582c2c, 0x2e341a1a, 0x2d361b1b, 0xb2dc6e6e, 0xeeb45a5a, 0xfb5ba0a0,
    0xf6a45252, 0x4d763b3b, 0x61b7d6d6, 0xce7db3b3, 0x7b522929, 0x3edde3e3, 0x715e2f2f, 0x97138484,
    0xf5a65353, 0x68b9d1d1, 0x00000000, 0x2cc1eded, 0x60402020, 0x1fe3fcfc, 0xc879b1b1, 0xedb65b5b,
    0xbed46a6a, 0x468dcbcb, 0xd967bebe, 0x4b723939, 0xde944a4a, 0xd4984c4c, 0xe8b05858, 0x4a85cfcf,
    0x6bbbd0d0, 0x2ac5efef, 0xe54faaaa, 0x16edfbfb, 0xc5864343, 0xd79a4d4d, 0x55663333, 0x94118585,
    0xcf8a4545, 0x10e9f9f9, 0x06040202, 0x81fe7f7f, 0xf0a05050, 0x44783c3c, 0xba259f9f, 0xe34ba8a8,
    0xf3a25151, 0xfe5da3a3, 0xc0804040, 0x8a058f8f, 0xad3f9292, 0xbc219d9d, 0x48703838, 0x04f1f5f5,
    0xdf63bcbc, 0xc177b6b6, 0x75afdada, 0x63422121, 0x30201010, 0x1ae5ffff, 0x0efdf3f3, 0x6dbfd2d2,
    0x4c81cdcd, 0x14180c0c, 0x35261313, 0x2fc3ecec, 0xe1be5f5f, 0xa2359797, 0xcc884444, 0x392e1717,
    0x5793c4c4, 0xf255a7a7, 0x82fc7e7e, 0x477a3d3d, 0xacc86464, 0xe7ba5d5d, 0x2b321919, 0x95e67373,
    0xa0c06060, 0x98198181, 0xd19e4f4f, 0x7fa3dcdc, 0x66442222, 0x7e542a2a, 0xab3b9090, 0x830b8888,
    0xca8c4646, 0x29c7eeee, 0xd36bb8b8, 0x3c281414, 0x79a7dede, 0xe2bc5e5e, 0x1d160b0b, 0x76addbdb,
    0x3bdbe0e0, 0x56643232, 0x4e743a3a, 0x1e140a0a, 0xdb924949, 0x0a0c0606, 0x6c482424, 0xe4b85c5c,
    0x5d9fc2c2, 0x6ebdd3d3, 0xef43acac, 0xa6c46262, 0xa8399191, 0xa4319595, 0x37d3e4e4, 0x8bf27979,
    0x32d5e7e7, 0x438bc8c8, 0x596e3737, 0xb7da6d6d, 0x8c018d8d, 0x64b1d5d5, 0xd29c4e4e, 0xe049a9a9,
    0xb4d86c6c, 0xfaac5656, 0x07f3f4f4, 0x25cfeaea, 0xafca6565, 0x8ef47a7a, 0xe947aeae, 0x18100808,
    0xd56fbaba, 0x88f07878, 0x6f4a2525, 0x725c2e2e, 0x24381c1c, 0xf157a6a6, 0xc773b4b4, 0x5197c6c6,
    0x23cbe8e8, 0x7ca1dddd, 0x9ce87474, 0x213e1f1f, 0xdd964b4b, 0xdc61bdbd, 0x860d8b8b, 0x850f8a8a,
    0x90e07070, 0x427c3e3e, 0xc471b5b5, 0xaacc6666, 0xd8904848, 0x05060303, 0x01f7f6f6, 0x121c0e0e,
    0xa3c26161, 0x5f6a3535, 0xf9ae5757, 0xd069b9b9, 0x91178686, 0x5899c1c1, 0x273a1d1d, 0xb9279e9e,
    0x38d9e1e1, 0x13ebf8f8, 0xb32b9898, 0x33221111, 0xbbd26969, 0x70a9d9d9, 0x89078e8e, 0xa7339494,
    0xb62d9b9b, 0x223c1e1e, 0x92158787, 0x20c9e9e9, 0x4987cece, 0xffaa5555, 0x78502828, 0x7aa5dfdf,
    0x8f038c8c, 0xf859a1a1, 0x80098989, 0x171a0d0d, 0xda65bfbf, 0x31d7e6e6, 0xc6844242, 0xb8d06868,
    0xc3824141, 0xb0299999, 0x775a2d2d, 0x111e0f0f, 0xcb7bb0b0, 0xfca85454, 0xd66dbbbb, 0x3a2c1616
};

static const uint32_t FIO___AES_TE2[256] = {
    0x63a5c663, 0x7c84f87c, 0x7799ee77, 0x7b8df67b, 0xf20dfff2, 0x6bbdd66b, 0x6fb1de6f, 0xc55491c5,
    0x30506030, 0x01030201, 0x67a9ce67, 0x2b7d562b, 0xfe19e7fe, 0xd762b5d7, 0xabe64dab, 0x769aec76,
    0xca458fca, 0x829d1f82, 0xc94089c9, 0x7d87fa7d, 0xfa15effa, 0x59ebb259, 0x47c98e47, 0xf00bfbf0,
    0xadec41ad, 0xd467b3d4, 0xa2fd5fa2, 0xafea45af, 0x9cbf239c, 0xa4f753a4, 0x7296e472, 0xc05b9bc0,
    0xb7c275b7, 0xfd1ce1fd, 0x93ae3d93, 0x266a4c26, 0x365a6c36, 0x3f417e3f, 0xf702f5f7, 0xcc4f83cc,
    0x345c6834, 0xa5f451a5, 0xe534d1e5, 0xf108f9f1, 0x7193e271, 0xd873abd8, 0x31536231, 0x153f2a15,
    0x040c0804, 0xc75295c7, 0x23654623, 0xc35e9dc3, 0x18283018, 0x96a13796, 0x050f0a05, 0x9ab52f9a,
    0x07090e07, 0x12362412, 0x809b1b80, 0xe23ddfe2, 0xeb26cdeb, 0x27694e27, 0xb2cd7fb2, 0x759fea75,
    0x091b1209, 0x839e1d83, 0x2c74582c, 0x1a2e341a, 0x1b2d361b, 0x6eb2dc6e, 0x5aeeb45a, 0xa0fb5ba0,
    0x52f6a452, 0x3b4d763b, 0xd661b7d6, 0xb3ce7db3, 0x297b5229, 0xe33edde3, 0x2f715e2f, 0x84971384,
    0x53f5a653, 0xd168b9d1, 0x00000000, 0xed2cc1ed, 0x20604020, 0xfc1fe3fc, 0xb1c879b1, 0x5bedb65b,
    0x6abed46a, 0xcb468dcb, 0xbed967be, 0x394b7239, 0x4ade944a, 0x4cd4984c, 0x58e8b058, 0xcf4a85cf,
    0xd06bbbd0, 0xef2ac5ef, 0xaae54faa, 0xfb16edfb, 0x43c58643, 0x4dd79a4d, 0x33556633, 0x85941185,
    0x45cf8a45, 0xf910e9f9, 0x02060402, 0x7f81fe7f, 0x50f0a050, 0x3c44783c, 0x9fba259f, 0xa8e34ba8,
    0x51f3a251, 0xa3fe5da3, 0x40c08040, 0x8f8a058f, 0x92ad3f92, 0x9dbc219d, 0x38487038, 0xf504f1f5,
    0xbcdf63bc, 0xb6c177b6, 0xda75afda, 0x21634221, 0x10302010, 0xff1ae5ff, 0xf30efdf3, 0xd26dbfd2,
    0xcd4c81cd, 0x0c14180c, 0x13352613, 0xec2fc3ec, 0x5fe1be5f, 0x97a23597, 0x44cc8844, 0x17392e17,
    0xc45793c4, 0xa7f255a7, 0x7e82fc7e, 0x3d477a3d, 0x64acc864, 0x5de7ba5d, 0x192b3219, 0x7395e673,
    0x60a0c060, 0x81981981, 0x4fd19e4f, 0xdc7fa3dc, 0x22664422, 0x2a7e542a, 0x90ab3b90, 0x88830b88,
    0x46ca8c46, 0xee29c7ee, 0xb8d36bb8, 0x143c2814, 0xde79a7de, 0x5ee2bc5e, 0x0b1d160b, 0xdb76addb,
    0xe03bdbe0, 0x32566432, 0x3a4e743a, 0x0a1e140a, 0x49db9249, 0x060a0c06, 0x246c4824, 0x5ce4b85c,
    0xc25d9fc2, 0xd36ebdd3, 0xacef43ac, 0x62a6c462, 0x91a83991, 0x95a43195, 0xe437d3e4, 0x798bf279,
    0xe732d5e7, 0xc8438bc8, 0x37596e37, 0x6db7da6d, 0x8d8c018d, 0xd564b1d5, 0x4ed29c4e, 0xa9e049a9,
    0x6cb4d86c, 0x56faac56, 0xf407f3f4, 0xea25cfea, 0x65afca65, 0x7a8ef47a, 0xaee947ae, 0x08181008,
    0xbad56fba, 0x7888f078, 0x256f4a25, 0x2e725c2e, 0x1c24381c, 0xa6f157a6, 0xb4c773b4, 0xc65197c6,
    0xe823cbe8, 0xdd7ca1dd, 0x749ce874, 0x1f213e1f, 0x4bdd964b, 0xbddc61bd, 0x8b860d8b, 0x8a850f8a,
    0x7090e070, 0x3e427c3e, 0xb5c471b5, 0x66aacc66, 0x48d89048, 0x03050603, 0xf601f7f6, 0x0e121c0e,
    0x61a3c261, 0x355f6a35, 0x57f9ae57, 0xb9d069b9, 0x86911786, 0xc15899c1, 0x1d273a1d, 0x9eb9279e,
    0xe138d9e1, 0xf813ebf8, 0x98b32b98, 0x11332211, 0x69bbd269, 0xd970a9d9, 0x8e89078e, 0x94a73394,
    0x9bb62d9b, 0x1e223c1e, 0x87921587, 0xe920c9e9, 0xce4987ce, 0x55ffaa55, 0x28785028, 0xdf7aa5df,
    0x8c8f038c, 0xa1f859a1, 0x89800989, 0x0d171a0d, 0xbfda65bf, 0xe631d7e6, 0x42c68442, 0x68b8d068,
    0x41c38241, 0x99b02999, 0x2d775a2d, 0x0f111e0f, 0xb0cb7bb0, 0x54fca854, 0xbbd66dbb, 0x163a2c16
};

static const uint32_t FIO___AES_TE3[256] = {
    0x6363a5c6, 0x7c7c84f8, 0x777799ee, 0x7b7b8df6, 0xf2f20dff, 0x6b6bbdd6, 0x6f6fb1de, 0xc5c55491,
    0x30305060, 0x01010302, 0x6767a9ce, 0x2b2b7d56, 0xfefe19e7, 0xd7d762b5, 0xababe64d, 0x76769aec,
    0xcaca458f, 0x82829d1f, 0xc9c94089, 0x7d7d87fa, 0xfafa15ef, 0x5959ebb2, 0x4747c98e, 0xf0f00bfb,
    0xadadec41, 0xd4d467b3, 0xa2a2fd5f, 0xafafea45, 0x9c9cbf23, 0xa4a4f753, 0x727296e4, 0xc0c05b9b,
    0xb7b7c275, 0xfdfd1ce1, 0x9393ae3d, 0x26266a4c, 0x36365a6c, 0x3f3f417e, 0xf7f702f5, 0xcccc4f83,
    0x34345c68, 0xa5a5f451, 0xe5e534d1, 0xf1f108f9, 0x717193e2, 0xd8d873ab, 0x31315362, 0x15153f2a,
    0x04040c08, 0xc7c75295, 0x23236546, 0xc3c35e9d, 0x18182830, 0x9696a137, 0x05050f0a, 0x9a9ab52f,
    0x0707090e, 0x12123624, 0x80809b1b, 0xe2e23ddf, 0xebeb26cd, 0x2727694e, 0xb2b2cd7f, 0x75759fea,
    0x09091b12, 0x83839e1d, 0x2c2c7458, 0x1a1a2e34, 0x1b1b2d36, 0x6e6eb2dc, 0x5a5aeeb4, 0xa0a0fb5b,
    0x5252f6a4, 0x3b3b4d76, 0xd6d661b7, 0xb3b3ce7d, 0x29297b52, 0xe3e33edd, 0x2f2f715e, 0x84849713,
    0x5353f5a6, 0xd1d168b9, 0x00000000, 0xeded2cc1, 0x20206040, 0xfcfc1fe3, 0xb1b1c879, 0x5b5bedb6,
    0x6a6abed4, 0xcbcb468d, 0xbebed967, 0x39394b72, 0x4a4ade94, 0x4c4cd498, 0x5858e8b0, 0xcfcf4a85,
    0xd0d06bbb, 0xefef2ac5, 0xaaaae54f, 0xfbfb16ed, 0x4343c586, 0x4d4dd79a, 0x33335566, 0x85859411,
    0x4545cf8a, 0xf9f910e9, 0x02020604, 0x7f7f81fe, 0x5050f0a0, 0x3c3c4478, 0x9f9fba25, 0xa8a8e34b,
    0x5151f3a2, 0xa3a3fe5d, 0x4040c080, 0x8f8f8a05, 0x9292ad3f, 0x9d9dbc21, 0x38384870, 0xf5f504f1,
    0xbcbcdf63, 0xb6b6c177, 0xdada75af, 0x21216342, 0x10103020, 0xffff1ae5, 0xf3f30efd, 0xd2d26dbf,
    0xcdcd4c81, 0x0c0c1418, 0x13133526, 0xecec2fc3, 0x5f5fe1be, 0x9797a235, 0x4444cc88, 0x1717392e,
    0xc4c45793, 0xa7a7f255, 0x7e7e82fc, 0x3d3d477a, 0x6464acc8, 0x5d5de7ba, 0x19192b32, 0x737395e6,
    0x6060a0c0, 0x81819819, 0x4f4fd19e, 0xdcdc7fa3, 0x22226644, 0x2a2a7e54, 0x9090ab3b, 0x8888830b,
    0x4646ca8c, 0xeeee29c7, 0xb8b8d36b, 0x14143c28, 0xdede79a7, 0x5e5ee2bc, 0x0b0b1d16, 0xdbdb76ad,
    0xe0e03bdb, 0x32325664, 0x3a3a4e74, 0x0a0a1e14, 0x4949db92, 0x06060a0c, 0x24246c48, 0x5c5ce4b8,
    0xc2c25d9f, 0xd3d36ebd, 0xacacef43, 0x6262a6c4, 0x9191a839, 0x9595a431, 0xe4e437d3, 0x79798bf2,
    0xe7e732d5, 0xc8c8438b, 0x3737596e, 0x6d6db7da, 0x8d8d8c01, 0xd5d564b1, 0x4e4ed29c, 0xa9a9e049,
    0x6c6cb4d8, 0x5656faac, 0xf4f407f3, 0xeaea25cf, 0x6565afca, 0x7a7a8ef4, 0xaeaee947, 0x08081810,
    0xbabad56f, 0x787888f0, 0x25256f4a, 0x2e2e725c, 0x1c1c2438, 0xa6a6f157, 0xb4b4c773, 0xc6c65197,
    0xe8e823cb, 0xdddd7ca1, 0x74749ce8, 0x1f1f213e, 0x4b4bdd96, 0xbdbddc61, 0x8b8b860d, 0x8a8a850f,
    0x707090e0, 0x3e3e427c, 0xb5b5c471, 0x6666aacc, 0x4848d890, 0x03030506, 0xf6f601f7, 0x0e0e121c,
    0x6161a3c2, 0x35355f6a, 0x5757f9ae, 0xb9b9d069, 0x86869117, 0xc1c15899, 0x1d1d273a, 0x9e9eb927,
    0xe1e138d9, 0xf8f813eb, 0x9898b32b, 0x11113322, 0x6969bbd2, 0xd9d970a9, 0x8e8e8907, 0x9494a733,
    0x9b9bb62d, 0x1e1e223c, 0x87879215, 0xe9e920c9, 0xcece4987, 0x5555ffaa, 0x28287850, 0xdfdf7aa5,
    0x8c8c8f03, 0xa1a1f859, 0x89898009, 0x0d0d171a, 0xbfbfda65, 0xe6e631d7, 0x4242c684, 0x6868b8d0,
    0x4141c382, 0x9999b029, 0x2d2d775a, 0x0f0f111e, 0xb0b0cb7b, 0x5454fca8, 0xbbbbd66d, 0x16163a2c
};
/* clang-format on */

/* SubWord: apply S-box to each byte of a 32-bit word */
FIO_IFUNC uint32_t fio___aes_subword(uint32_t w) {
  return ((uint32_t)FIO___AES_SBOX[(w >> 0) & 0xFF] << 0) |
         ((uint32_t)FIO___AES_SBOX[(w >> 8) & 0xFF] << 8) |
         ((uint32_t)FIO___AES_SBOX[(w >> 16) & 0xFF] << 16) |
         ((uint32_t)FIO___AES_SBOX[(w >> 24) & 0xFF] << 24);
}

FIO_IFUNC void fio___aes128_key_expand(uint32_t *w, const uint8_t key[16]) {
  for (int i = 0; i < 4; ++i)
    w[i] = fio_buf2u32_be(key + 4 * i);
  for (int i = 4; i < 44; ++i) {
    uint32_t tmp = w[i - 1];
    if ((i & 3) == 0)
      tmp = fio___aes_subword(fio_rrot32(tmp, 24)) ^
            ((uint32_t)FIO___AES_RCON[i / 4] << 24);
    w[i] = w[i - 4] ^ tmp;
  }
}

FIO_IFUNC void fio___aes256_key_expand(uint32_t *w, const uint8_t key[32]) {
  for (int i = 0; i < 8; ++i)
    w[i] = fio_buf2u32_be(key + 4 * i);
  for (int i = 8; i < 60; ++i) {
    uint32_t tmp = w[i - 1];
    if ((i & 7) == 0)
      tmp = fio___aes_subword(fio_rrot32(tmp, 24)) ^
            ((uint32_t)FIO___AES_RCON[i / 8] << 24);
    else if ((i & 7) == 4)
      tmp = fio___aes_subword(tmp);
    w[i] = w[i - 8] ^ tmp;
  }
}

FIO_IFUNC void fio___aes_encrypt_round(uint32_t *state, const uint32_t *rk) {
  uint32_t s0 = state[0], s1 = state[1], s2 = state[2], s3 = state[3];
  state[0] =
      FIO___AES_TE0[(s0 >> 24) & 0xFF] ^ FIO___AES_TE1[(s1 >> 16) & 0xFF] ^
      FIO___AES_TE2[(s2 >> 8) & 0xFF] ^ FIO___AES_TE3[(s3 >> 0) & 0xFF] ^ rk[0];
  state[1] =
      FIO___AES_TE0[(s1 >> 24) & 0xFF] ^ FIO___AES_TE1[(s2 >> 16) & 0xFF] ^
      FIO___AES_TE2[(s3 >> 8) & 0xFF] ^ FIO___AES_TE3[(s0 >> 0) & 0xFF] ^ rk[1];
  state[2] =
      FIO___AES_TE0[(s2 >> 24) & 0xFF] ^ FIO___AES_TE1[(s3 >> 16) & 0xFF] ^
      FIO___AES_TE2[(s0 >> 8) & 0xFF] ^ FIO___AES_TE3[(s1 >> 0) & 0xFF] ^ rk[2];
  state[3] =
      FIO___AES_TE0[(s3 >> 24) & 0xFF] ^ FIO___AES_TE1[(s0 >> 16) & 0xFF] ^
      FIO___AES_TE2[(s1 >> 8) & 0xFF] ^ FIO___AES_TE3[(s2 >> 0) & 0xFF] ^ rk[3];
}

FIO_IFUNC void fio___aes_encrypt_final_round(uint32_t *state,
                                             const uint32_t *rk) {
  uint32_t s0 = state[0], s1 = state[1], s2 = state[2], s3 = state[3];
  state[0] = ((uint32_t)FIO___AES_SBOX[(s0 >> 24) & 0xFF] << 24) ^
             ((uint32_t)FIO___AES_SBOX[(s1 >> 16) & 0xFF] << 16) ^
             ((uint32_t)FIO___AES_SBOX[(s2 >> 8) & 0xFF] << 8) ^
             ((uint32_t)FIO___AES_SBOX[(s3 >> 0) & 0xFF] << 0) ^ rk[0];
  state[1] = ((uint32_t)FIO___AES_SBOX[(s1 >> 24) & 0xFF] << 24) ^
             ((uint32_t)FIO___AES_SBOX[(s2 >> 16) & 0xFF] << 16) ^
             ((uint32_t)FIO___AES_SBOX[(s3 >> 8) & 0xFF] << 8) ^
             ((uint32_t)FIO___AES_SBOX[(s0 >> 0) & 0xFF] << 0) ^ rk[1];
  state[2] = ((uint32_t)FIO___AES_SBOX[(s2 >> 24) & 0xFF] << 24) ^
             ((uint32_t)FIO___AES_SBOX[(s3 >> 16) & 0xFF] << 16) ^
             ((uint32_t)FIO___AES_SBOX[(s0 >> 8) & 0xFF] << 8) ^
             ((uint32_t)FIO___AES_SBOX[(s1 >> 0) & 0xFF] << 0) ^ rk[2];
  state[3] = ((uint32_t)FIO___AES_SBOX[(s3 >> 24) & 0xFF] << 24) ^
             ((uint32_t)FIO___AES_SBOX[(s0 >> 16) & 0xFF] << 16) ^
             ((uint32_t)FIO___AES_SBOX[(s1 >> 8) & 0xFF] << 8) ^
             ((uint32_t)FIO___AES_SBOX[(s2 >> 0) & 0xFF] << 0) ^ rk[3];
}

FIO_IFUNC void fio___aes128_encrypt_block(uint8_t out[16],
                                          const uint8_t in[16],
                                          const uint32_t *rk) {
  uint32_t state[4];
  fio_memcpy16(state, in);
  state[0] = fio_lton32(state[0]) ^ rk[0];
  state[1] = fio_lton32(state[1]) ^ rk[1];
  state[2] = fio_lton32(state[2]) ^ rk[2];
  state[3] = fio_lton32(state[3]) ^ rk[3];
  for (int round = 1; round < 10; ++round)
    fio___aes_encrypt_round(state, rk + round * 4);
  fio___aes_encrypt_final_round(state, rk + 40);
  state[0] = fio_lton32(state[0]);
  state[1] = fio_lton32(state[1]);
  state[2] = fio_lton32(state[2]);
  state[3] = fio_lton32(state[3]);
  fio_memcpy16(out, state);
}

FIO_IFUNC void fio___aes256_encrypt_block(uint8_t out[16],
                                          const uint8_t in[16],
                                          const uint32_t *rk) {
  uint32_t state[4];
  fio_memcpy16(state, in);
  state[0] = fio_lton32(state[0]) ^ rk[0];
  state[1] = fio_lton32(state[1]) ^ rk[1];
  state[2] = fio_lton32(state[2]) ^ rk[2];
  state[3] = fio_lton32(state[3]) ^ rk[3];
  for (int round = 1; round < 14; ++round)
    fio___aes_encrypt_round(state, rk + round * 4);
  fio___aes_encrypt_final_round(state, rk + 56);
  state[0] = fio_lton32(state[0]);
  state[1] = fio_lton32(state[1]);
  state[2] = fio_lton32(state[2]);
  state[3] = fio_lton32(state[3]);
  fio_memcpy16(out, state);
}

/* 4-bit table-based GHASH using Shoup's method
 *
 * We precompute 16 entries: M[i] = i * H for i=0..15
 * This gives us 256 bytes of tables and 32 iterations per block.
 *
 * We process nibbles from least significant to most significant:
 *   Z = Z * x^4 + nibble * H
 *
 * The multiplication by x^4 is a right shift by 4 bits with reduction.
 * We use a 16-entry reduction table for the nibble that falls off.
 */
typedef struct {
  uint64_t hl[16]; /* Low 64 bits of i*H for i=0..15 */
  uint64_t hh[16]; /* High 64 bits of i*H for i=0..15 */
} fio___gcm_htable_s;

/* Reduction table: when shifting right by 4, the low nibble falls off.
 * Entry i contains the XOR value for the high word when nibble i falls off.
 * Computed as: i * (x^128 mod P) where P = x^128 + x^7 + x^2 + x + 1 */
/* clang-format off */
static const uint64_t FIO___GCM_REDUCE4[16] = {
    0x0000000000000000ULL, 0x1C20000000000000ULL,
    0x3840000000000000ULL, 0x2460000000000000ULL,
    0x7080000000000000ULL, 0x6CA0000000000000ULL,
    0x48C0000000000000ULL, 0x54E0000000000000ULL,
    0xE100000000000000ULL, 0xFD20000000000000ULL,
    0xD940000000000000ULL, 0xC560000000000000ULL,
    0x9180000000000000ULL, 0x8DA0000000000000ULL,
    0xA9C0000000000000ULL, 0xB5E0000000000000ULL
};
/* clang-format on */

/* Precompute the 16-entry multiplication table: M[i] = i * H */
FIO_IFUNC void fio___gcm_precompute_htable(fio___gcm_htable_s *ctx,
                                           const uint8_t h[16]) {
  uint64_t h0 = fio_buf2u64_be(h);
  uint64_t h1 = fio_buf2u64_be(h + 8);

  ctx->hh[0] = 0;
  ctx->hl[0] = 0;
  ctx->hh[8] = h0;
  ctx->hl[8] = h1;

  /* Powers of x times H: M[4] = x*H, M[2] = x^2*H, M[1] = x^3*H */
  uint64_t carry = h1 & 1;
  ctx->hl[4] = (h1 >> 1) | (h0 << 63);
  ctx->hh[4] = (h0 >> 1) ^ (carry ? 0xE100000000000000ULL : 0);

  carry = ctx->hl[4] & 1;
  ctx->hl[2] = (ctx->hl[4] >> 1) | (ctx->hh[4] << 63);
  ctx->hh[2] = (ctx->hh[4] >> 1) ^ (carry ? 0xE100000000000000ULL : 0);

  carry = ctx->hl[2] & 1;
  ctx->hl[1] = (ctx->hl[2] >> 1) | (ctx->hh[2] << 63);
  ctx->hh[1] = (ctx->hh[2] >> 1) ^ (carry ? 0xE100000000000000ULL : 0);

  /* Build remaining entries using XOR (linearity of GF multiplication) */
  ctx->hh[3] = ctx->hh[1] ^ ctx->hh[2];
  ctx->hl[3] = ctx->hl[1] ^ ctx->hl[2];
  ctx->hh[5] = ctx->hh[1] ^ ctx->hh[4];
  ctx->hl[5] = ctx->hl[1] ^ ctx->hl[4];
  ctx->hh[6] = ctx->hh[2] ^ ctx->hh[4];
  ctx->hl[6] = ctx->hl[2] ^ ctx->hl[4];
  ctx->hh[7] = ctx->hh[1] ^ ctx->hh[6];
  ctx->hl[7] = ctx->hl[1] ^ ctx->hl[6];
  ctx->hh[9] = ctx->hh[1] ^ ctx->hh[8];
  ctx->hl[9] = ctx->hl[1] ^ ctx->hl[8];
  ctx->hh[10] = ctx->hh[2] ^ ctx->hh[8];
  ctx->hl[10] = ctx->hl[2] ^ ctx->hl[8];
  ctx->hh[11] = ctx->hh[3] ^ ctx->hh[8];
  ctx->hl[11] = ctx->hl[3] ^ ctx->hl[8];
  ctx->hh[12] = ctx->hh[4] ^ ctx->hh[8];
  ctx->hl[12] = ctx->hl[4] ^ ctx->hl[8];
  ctx->hh[13] = ctx->hh[5] ^ ctx->hh[8];
  ctx->hl[13] = ctx->hl[5] ^ ctx->hl[8];
  ctx->hh[14] = ctx->hh[6] ^ ctx->hh[8];
  ctx->hl[14] = ctx->hl[6] ^ ctx->hl[8];
  ctx->hh[15] = ctx->hh[7] ^ ctx->hh[8];
  ctx->hl[15] = ctx->hl[7] ^ ctx->hl[8];
}

/* GHASH multiplication: result = x * H using 4-bit table
 * Process nibbles from byte 15 down to byte 0, low nibble first
 *
 * Unrolled for better performance - processes 2 bytes (4 nibbles) per iteration
 */
FIO_IFUNC void fio___gcm_ghash_mult(uint64_t z[2],
                                    const uint8_t x[16],
                                    const fio___gcm_htable_s *ctx) {
  uint64_t z0 = 0, z1 = 0;
  uint64_t rem;

  /* Unroll: process 2 bytes at a time */
  for (int i = 14; i >= 0; i -= 2) {
    uint8_t b0 = x[i + 1]; /* Lower byte first */
    uint8_t b1 = x[i];

    /* Byte 0, low nibble */
    rem = z1 & 0xF;
    z1 = (z1 >> 4) | (z0 << 60);
    z0 = (z0 >> 4) ^ FIO___GCM_REDUCE4[rem];
    z0 ^= ctx->hh[b0 & 0xF];
    z1 ^= ctx->hl[b0 & 0xF];

    /* Byte 0, high nibble */
    rem = z1 & 0xF;
    z1 = (z1 >> 4) | (z0 << 60);
    z0 = (z0 >> 4) ^ FIO___GCM_REDUCE4[rem];
    z0 ^= ctx->hh[b0 >> 4];
    z1 ^= ctx->hl[b0 >> 4];

    /* Byte 1, low nibble */
    rem = z1 & 0xF;
    z1 = (z1 >> 4) | (z0 << 60);
    z0 = (z0 >> 4) ^ FIO___GCM_REDUCE4[rem];
    z0 ^= ctx->hh[b1 & 0xF];
    z1 ^= ctx->hl[b1 & 0xF];

    /* Byte 1, high nibble */
    rem = z1 & 0xF;
    z1 = (z1 >> 4) | (z0 << 60);
    z0 = (z0 >> 4) ^ FIO___GCM_REDUCE4[rem];
    z0 ^= ctx->hh[b1 >> 4];
    z1 ^= ctx->hl[b1 >> 4];
  }

  z[0] = z0;
  z[1] = z1;
}

/* GHASH a block: tag = (tag XOR block) * H
 * Optimized to use 64-bit XOR operations */
FIO_IFUNC void fio___gcm_ghash_block(uint64_t tag[2],
                                     const uint8_t block[16],
                                     const fio___gcm_htable_s *ctx) {
  uint8_t tmp[16];
  uint64_t b0 = fio_buf2u64_be(block);
  uint64_t b1 = fio_buf2u64_be(block + 8);
  fio_u2buf64_be(tmp, tag[0] ^ b0);
  fio_u2buf64_be(tmp + 8, tag[1] ^ b1);
  fio___gcm_ghash_mult(tag, tmp, ctx);
}

/* GHASH over data */
FIO_IFUNC void fio___gcm_ghash(uint64_t tag[2],
                               const fio___gcm_htable_s *ctx,
                               const uint8_t *data,
                               size_t len) {
  while (len >= 16) {
    fio___gcm_ghash_block(tag, data, ctx);
    data += 16;
    len -= 16;
  }
  if (len > 0) {
    uint8_t block[16] = {0};
    FIO_MEMCPY(block, data, len);
    fio___gcm_ghash_block(tag, block, ctx);
  }
}

FIO_IFUNC void fio___gcm_inc_counter(uint8_t counter[16]) {
  uint32_t c = fio_buf2u32_be(counter + 12);
  c++;
  fio_u2buf32_be(counter + 12, c);
}

SFUNC void fio_aes128_gcm_enc(void *restrict mac,
                              void *restrict data,
                              size_t len,
                              const void *ad,
                              size_t adlen,
                              const void *key,
                              const void *nonce) {
  uint32_t rk[44];
  uint8_t h[16] = {0};
  fio___gcm_htable_s htbl;
  uint8_t j0[16];
  uint8_t counter[16];
  uint8_t keystream[16];
  uint64_t tag[2] = {0, 0};
  uint8_t len_block[16];
  uint8_t *p = (uint8_t *)data;
  size_t orig_len = len;

  fio___aes128_key_expand(rk, (const uint8_t *)key);
  fio___aes128_encrypt_block(h, h, rk);
  fio___gcm_precompute_htable(&htbl, h);

  FIO_MEMCPY(j0, nonce, 12);
  j0[12] = 0;
  j0[13] = 0;
  j0[14] = 0;
  j0[15] = 1;

  fio___gcm_ghash(tag, &htbl, (const uint8_t *)ad, adlen);

  FIO_MEMCPY(counter, j0, 16);
  while (len >= 16) {
    fio___gcm_inc_counter(counter);
    fio___aes128_encrypt_block(keystream, counter, rk);
    /* XOR 16 bytes - use byte-by-byte to avoid alignment issues */
    for (size_t i = 0; i < 16; ++i)
      p[i] ^= keystream[i];
    fio___gcm_ghash_block(tag, p, &htbl);
    p += 16;
    len -= 16;
  }
  if (len > 0) {
    fio___gcm_inc_counter(counter);
    fio___aes128_encrypt_block(keystream, counter, rk);
    for (size_t i = 0; i < len; ++i)
      p[i] ^= keystream[i];
    uint8_t block[16] = {0};
    FIO_MEMCPY(block, p, len);
    fio___gcm_ghash_block(tag, block, &htbl);
  }

  FIO_MEMSET(len_block, 0, 16);
  fio_u2buf64_be(len_block, (uint64_t)adlen * 8);
  fio_u2buf64_be(len_block + 8, (uint64_t)orig_len * 8);
  fio___gcm_ghash_block(tag, len_block, &htbl);

  fio___aes128_encrypt_block(keystream, j0, rk);
  fio_u2buf64_be((uint8_t *)mac, tag[0]);
  fio_u2buf64_be((uint8_t *)mac + 8, tag[1]);
  for (int i = 0; i < 16; ++i)
    ((uint8_t *)mac)[i] ^= keystream[i];
  /* Clear sensitive data */
  fio_secure_zero(rk, sizeof(rk));
  fio_secure_zero(&htbl, sizeof(htbl));
  fio_secure_zero(j0, sizeof(j0));
  fio_secure_zero(counter, sizeof(counter));
  fio_secure_zero(keystream, sizeof(keystream));
  fio_secure_zero(tag, sizeof(tag));
}

SFUNC void fio_aes256_gcm_enc(void *restrict mac,
                              void *restrict data,
                              size_t len,
                              const void *ad,
                              size_t adlen,
                              const void *key,
                              const void *nonce) {
  uint32_t rk[60];
  uint8_t h[16] = {0};
  fio___gcm_htable_s htbl;
  uint8_t j0[16];
  uint8_t counter[16];
  uint8_t keystream[16];
  uint64_t tag[2] = {0, 0};
  uint8_t len_block[16];
  uint8_t *p = (uint8_t *)data;
  size_t orig_len = len;

  fio___aes256_key_expand(rk, (const uint8_t *)key);
  fio___aes256_encrypt_block(h, h, rk);
  fio___gcm_precompute_htable(&htbl, h);

  FIO_MEMCPY(j0, nonce, 12);
  j0[12] = 0;
  j0[13] = 0;
  j0[14] = 0;
  j0[15] = 1;

  fio___gcm_ghash(tag, &htbl, (const uint8_t *)ad, adlen);

  FIO_MEMCPY(counter, j0, 16);
  while (len >= 16) {
    fio___gcm_inc_counter(counter);
    fio___aes256_encrypt_block(keystream, counter, rk);
    /* XOR 16 bytes - use byte-by-byte to avoid alignment issues */
    for (size_t i = 0; i < 16; ++i)
      p[i] ^= keystream[i];
    fio___gcm_ghash_block(tag, p, &htbl);
    p += 16;
    len -= 16;
  }
  if (len > 0) {
    fio___gcm_inc_counter(counter);
    fio___aes256_encrypt_block(keystream, counter, rk);
    for (size_t i = 0; i < len; ++i)
      p[i] ^= keystream[i];
    uint8_t block[16] = {0};
    FIO_MEMCPY(block, p, len);
    fio___gcm_ghash_block(tag, block, &htbl);
  }

  FIO_MEMSET(len_block, 0, 16);
  fio_u2buf64_be(len_block, (uint64_t)adlen * 8);
  fio_u2buf64_be(len_block + 8, (uint64_t)orig_len * 8);
  fio___gcm_ghash_block(tag, len_block, &htbl);

  fio___aes256_encrypt_block(keystream, j0, rk);
  fio_u2buf64_be((uint8_t *)mac, tag[0]);
  fio_u2buf64_be((uint8_t *)mac + 8, tag[1]);
  for (int i = 0; i < 16; ++i)
    ((uint8_t *)mac)[i] ^= keystream[i];
  /* Clear sensitive data */
  fio_secure_zero(rk, sizeof(rk));
  fio_secure_zero(&htbl, sizeof(htbl));
  fio_secure_zero(j0, sizeof(j0));
  fio_secure_zero(counter, sizeof(counter));
  fio_secure_zero(keystream, sizeof(keystream));
  fio_secure_zero(tag, sizeof(tag));
}

SFUNC int fio_aes128_gcm_dec(void *restrict mac,
                             void *restrict data,
                             size_t len,
                             const void *ad,
                             size_t adlen,
                             const void *key,
                             const void *nonce) {
  uint32_t rk[44];
  uint8_t h[16] = {0};
  fio___gcm_htable_s htbl;
  uint8_t j0[16];
  uint8_t counter[16];
  uint8_t keystream[16];
  uint64_t tag[2] = {0, 0};
  uint8_t len_block[16];
  uint8_t computed_mac[16];
  uint8_t *p = (uint8_t *)data;
  size_t orig_len = len;

  fio___aes128_key_expand(rk, (const uint8_t *)key);
  fio___aes128_encrypt_block(h, h, rk);
  fio___gcm_precompute_htable(&htbl, h);

  FIO_MEMCPY(j0, nonce, 12);
  j0[12] = 0;
  j0[13] = 0;
  j0[14] = 0;
  j0[15] = 1;

  fio___gcm_ghash(tag, &htbl, (const uint8_t *)ad, adlen);
  fio___gcm_ghash(tag, &htbl, p, orig_len);

  FIO_MEMSET(len_block, 0, 16);
  fio_u2buf64_be(len_block, (uint64_t)adlen * 8);
  fio_u2buf64_be(len_block + 8, (uint64_t)orig_len * 8);
  fio___gcm_ghash_block(tag, len_block, &htbl);

  fio___aes128_encrypt_block(keystream, j0, rk);
  fio_u2buf64_be(computed_mac, tag[0]);
  fio_u2buf64_be(computed_mac + 8, tag[1]);
  for (int i = 0; i < 16; ++i)
    computed_mac[i] ^= keystream[i];

  if (!fio_ct_is_eq(computed_mac, mac, 16)) {
    fio_secure_zero(computed_mac, sizeof(computed_mac));
    fio_secure_zero(rk, sizeof(rk));
    fio_secure_zero(&htbl, sizeof(htbl));
    fio_secure_zero(j0, sizeof(j0));
    fio_secure_zero(counter, sizeof(counter));
    fio_secure_zero(keystream, sizeof(keystream));
    fio_secure_zero(tag, sizeof(tag));
    return -1;
  }
  fio_secure_zero(computed_mac, sizeof(computed_mac));

  FIO_MEMCPY(counter, j0, 16);
  while (len >= 16) {
    fio___gcm_inc_counter(counter);
    fio___aes128_encrypt_block(keystream, counter, rk);
    /* Use 64-bit XOR for better performance */
    uint64_t *p64 = (uint64_t *)p;
    uint64_t *ks64 = (uint64_t *)keystream;
    p64[0] ^= ks64[0];
    p64[1] ^= ks64[1];
    p += 16;
    len -= 16;
  }
  if (len > 0) {
    fio___gcm_inc_counter(counter);
    fio___aes128_encrypt_block(keystream, counter, rk);
    for (size_t i = 0; i < len; ++i)
      p[i] ^= keystream[i];
  }
  /* Clear sensitive data */
  fio_secure_zero(rk, sizeof(rk));
  fio_secure_zero(&htbl, sizeof(htbl));
  fio_secure_zero(j0, sizeof(j0));
  fio_secure_zero(counter, sizeof(counter));
  fio_secure_zero(keystream, sizeof(keystream));
  fio_secure_zero(tag, sizeof(tag));
  return 0;
}

SFUNC int fio_aes256_gcm_dec(void *restrict mac,
                             void *restrict data,
                             size_t len,
                             const void *ad,
                             size_t adlen,
                             const void *key,
                             const void *nonce) {
  uint32_t rk[60];
  uint8_t h[16] = {0};
  fio___gcm_htable_s htbl;
  uint8_t j0[16];
  uint8_t counter[16];
  uint8_t keystream[16];
  uint64_t tag[2] = {0, 0};
  uint8_t len_block[16];
  uint8_t computed_mac[16];
  uint8_t *p = (uint8_t *)data;
  size_t orig_len = len;

  fio___aes256_key_expand(rk, (const uint8_t *)key);
  fio___aes256_encrypt_block(h, h, rk);
  fio___gcm_precompute_htable(&htbl, h);

  FIO_MEMCPY(j0, nonce, 12);
  j0[12] = 0;
  j0[13] = 0;
  j0[14] = 0;
  j0[15] = 1;

  fio___gcm_ghash(tag, &htbl, (const uint8_t *)ad, adlen);
  fio___gcm_ghash(tag, &htbl, p, orig_len);

  FIO_MEMSET(len_block, 0, 16);
  fio_u2buf64_be(len_block, (uint64_t)adlen * 8);
  fio_u2buf64_be(len_block + 8, (uint64_t)orig_len * 8);
  fio___gcm_ghash_block(tag, len_block, &htbl);

  fio___aes256_encrypt_block(keystream, j0, rk);
  fio_u2buf64_be(computed_mac, tag[0]);
  fio_u2buf64_be(computed_mac + 8, tag[1]);
  for (int i = 0; i < 16; ++i)
    computed_mac[i] ^= keystream[i];

  if (!fio_ct_is_eq(computed_mac, mac, 16)) {
    fio_secure_zero(computed_mac, sizeof(computed_mac));
    fio_secure_zero(rk, sizeof(rk));
    fio_secure_zero(&htbl, sizeof(htbl));
    fio_secure_zero(j0, sizeof(j0));
    fio_secure_zero(counter, sizeof(counter));
    fio_secure_zero(keystream, sizeof(keystream));
    fio_secure_zero(tag, sizeof(tag));
    return -1;
  }
  fio_secure_zero(computed_mac, sizeof(computed_mac));

  FIO_MEMCPY(counter, j0, 16);
  while (len >= 16) {
    fio___gcm_inc_counter(counter);
    fio___aes256_encrypt_block(keystream, counter, rk);
    /* Use 64-bit XOR for better performance */
    uint64_t *p64 = (uint64_t *)p;
    uint64_t *ks64 = (uint64_t *)keystream;
    p64[0] ^= ks64[0];
    p64[1] ^= ks64[1];
    p += 16;
    len -= 16;
  }
  if (len > 0) {
    fio___gcm_inc_counter(counter);
    fio___aes256_encrypt_block(keystream, counter, rk);
    for (size_t i = 0; i < len; ++i)
      p[i] ^= keystream[i];
  }
  /* Clear sensitive data */
  fio_secure_zero(rk, sizeof(rk));
  fio_secure_zero(&htbl, sizeof(htbl));
  fio_secure_zero(j0, sizeof(j0));
  fio_secure_zero(counter, sizeof(counter));
  fio_secure_zero(keystream, sizeof(keystream));
  fio_secure_zero(tag, sizeof(tag));
  return 0;
}

#endif /* Hardware vs Software implementation */

/* *****************************************************************************
Module Cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_AES
#endif /* FIO_AES */
