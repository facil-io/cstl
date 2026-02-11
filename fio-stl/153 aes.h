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
