/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_SHA2               /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                                    SHA 2
                        SHA-256 / SHA-512 and variations



Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_SHA2) && !defined(H___FIO_SHA2___H)
#define H___FIO_SHA2___H
/* *****************************************************************************
SHA 2 API
***************************************************************************** */

/** Streaming SHA-256 type. */
typedef struct {
  fio_u256 hash;
  fio_u512 cache;
  uint64_t total_len;
} fio_sha256_s;

/** A simple, non streaming, implementation of the SHA-256 hashing algorithm. */
FIO_IFUNC fio_u256 fio_sha256(const void *data, uint64_t len);

/** initializes a fio_u256 so the hash can consume streaming data. */
FIO_IFUNC fio_sha256_s fio_sha256_init(void);
/** Feed data into the hash */
SFUNC void fio_sha256_consume(fio_sha256_s *h, const void *data, uint64_t len);
/** finalizes a fio_u256 with the SHA 256 hash. */
SFUNC fio_u256 fio_sha256_finalize(fio_sha256_s *h);

/** Streaming SHA-512 type. */
typedef struct {
  fio_u512 hash;
  fio_u1024 cache;
  uint64_t total_len;
} fio_sha512_s;

/** A simple, non streaming, implementation of the SHA-512 hashing algorithm. */
FIO_IFUNC fio_u512 fio_sha512(const void *data, uint64_t len);

/** initializes a fio_u512 so the hash can consume streaming data. */
FIO_IFUNC fio_sha512_s fio_sha512_init(void);
/** Feed data into the hash */
SFUNC void fio_sha512_consume(fio_sha512_s *h, const void *data, uint64_t len);
/** finalizes a fio_u512 with the SHA 512 hash. */
SFUNC fio_u512 fio_sha512_finalize(fio_sha512_s *h);

/* *****************************************************************************
Implementation - static / inline functions.
***************************************************************************** */

/** initializes a fio_u256 so the hash can be consumed. */
FIO_IFUNC fio_sha256_s fio_sha256_init(void) {
  fio_sha256_s h = {.hash.u32 = {0x6A09E667ULL,
                                 0xBB67AE85ULL,
                                 0x3C6EF372ULL,
                                 0xA54FF53AULL,
                                 0x510E527FULL,
                                 0x9B05688CULL,
                                 0x1F83D9ABULL,
                                 0x5BE0CD19ULL}};
  return h;
}

/** A simple, non streaming, implementation of the SHA-256 hashing algorithm. */
FIO_IFUNC fio_u256 fio_sha256(const void *data, uint64_t len) {
  fio_sha256_s h = fio_sha256_init();
  fio_sha256_consume(&h, data, len);
  return fio_sha256_finalize(&h);
}

/** initializes a fio_u256 so the hash can be consumed. */
FIO_IFUNC fio_sha512_s fio_sha512_init(void) {
  fio_sha512_s h = {.hash.u64 = {0x6A09E667F3BCC908ULL,
                                 0xBB67AE8584CAA73BULL,
                                 0x3C6EF372FE94F82BULL,
                                 0xA54FF53A5F1D36F1ULL,
                                 0x510E527FADE682D1ULL,
                                 0x9B05688C2B3E6C1FULL,
                                 0x1F83D9ABFB41BD6BULL,
                                 0x5BE0CD19137E2179ULL}};
  return h;
}

/** A simple, non streaming, implementation of the SHA-256 hashing algorithm. */
FIO_IFUNC fio_u512 fio_sha512(const void *data, uint64_t len) {
  fio_sha512_s h = fio_sha512_init();
  fio_sha512_consume(&h, data, len);
  return fio_sha512_finalize(&h);
}

/* *****************************************************************************
Implementation - possibly externed functions.
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/* *****************************************************************************
Implementation - SHA-256
***************************************************************************** */

FIO_IFUNC void fio___sha256_round(fio_u256 *h, const uint8_t *block) {
#if FIO___HAS_X86_SHA_INTRIN
  /* Code adjusted from:
   * https://github.com/noloader/SHA-Intrinsics/blob/master/sha256-x86.c
   * Credit to Jeffrey Walton.
   */
  __m128i state0, state1;
  __m128i msg, tmp;
  __m128i msg0, msg1, msg2, msg3;
  __m128i abef_save, cdgh_save;
  const __m128i shuf_mask =
      _mm_set_epi8(12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3);

  /* Load initial values */
  tmp = _mm_loadu_si128((const __m128i *)&h->u32[0]);
  state1 = _mm_loadu_si128((const __m128i *)&h->u32[4]);

  tmp = _mm_shuffle_epi32(tmp, 0xB1);          /* CDAB */
  state1 = _mm_shuffle_epi32(state1, 0x1B);    /* EFGH */
  state0 = _mm_alignr_epi8(tmp, state1, 8);    /* ABEF */
  state1 = _mm_blend_epi16(state1, tmp, 0xF0); /* CDGH */

  /* Save current state */
  abef_save = state0;
  cdgh_save = state1;

  /* Rounds 0-3 */
  msg = _mm_loadu_si128((const __m128i *)(block + 0));
  msg0 = _mm_shuffle_epi8(msg, shuf_mask);
  msg = _mm_add_epi32(
      msg0,
      _mm_set_epi64x(0xE9B5DBA5B5C0FBCFULL, 0x71374491428A2F98ULL));
  state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
  msg = _mm_shuffle_epi32(msg, 0x0E);
  state0 = _mm_sha256rnds2_epu32(state0, state1, msg);

  /* Rounds 4-7 */
  msg1 = _mm_loadu_si128((const __m128i *)(block + 16));
  msg1 = _mm_shuffle_epi8(msg1, shuf_mask);
  msg = _mm_add_epi32(
      msg1,
      _mm_set_epi64x(0xAB1C5ED5923F82A4ULL, 0x59F111F13956C25BULL));
  state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
  msg = _mm_shuffle_epi32(msg, 0x0E);
  state0 = _mm_sha256rnds2_epu32(state0, state1, msg);
  msg0 = _mm_sha256msg1_epu32(msg0, msg1);

  /* Rounds 8-11 */
  msg2 = _mm_loadu_si128((const __m128i *)(block + 32));
  msg2 = _mm_shuffle_epi8(msg2, shuf_mask);
  msg = _mm_add_epi32(
      msg2,
      _mm_set_epi64x(0x550C7DC3243185BEULL, 0x12835B01D807AA98ULL));
  state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
  msg = _mm_shuffle_epi32(msg, 0x0E);
  state0 = _mm_sha256rnds2_epu32(state0, state1, msg);
  msg1 = _mm_sha256msg1_epu32(msg1, msg2);

  /* Rounds 12-15 */
  msg3 = _mm_loadu_si128((const __m128i *)(block + 48));
  msg3 = _mm_shuffle_epi8(msg3, shuf_mask);
  msg = _mm_add_epi32(
      msg3,
      _mm_set_epi64x(0xC19BF1749BDC06A7ULL, 0x80DEB1FE72BE5D74ULL));
  state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
  tmp = _mm_alignr_epi8(msg3, msg2, 4);
  msg0 = _mm_add_epi32(msg0, tmp);
  msg0 = _mm_sha256msg2_epu32(msg0, msg3);
  msg = _mm_shuffle_epi32(msg, 0x0E);
  state0 = _mm_sha256rnds2_epu32(state0, state1, msg);
  msg2 = _mm_sha256msg1_epu32(msg2, msg3);

  /* Rounds 16-19 */
  msg = _mm_add_epi32(
      msg0,
      _mm_set_epi64x(0x240CA1CC0FC19DC6ULL, 0xEFBE4786E49B69C1ULL));
  state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
  tmp = _mm_alignr_epi8(msg0, msg3, 4);
  msg1 = _mm_add_epi32(msg1, tmp);
  msg1 = _mm_sha256msg2_epu32(msg1, msg0);
  msg = _mm_shuffle_epi32(msg, 0x0E);
  state0 = _mm_sha256rnds2_epu32(state0, state1, msg);
  msg3 = _mm_sha256msg1_epu32(msg3, msg0);

  /* Rounds 20-23 */
  msg = _mm_add_epi32(
      msg1,
      _mm_set_epi64x(0x76F988DA5CB0A9DCULL, 0x4A7484AA2DE92C6FULL));
  state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
  tmp = _mm_alignr_epi8(msg1, msg0, 4);
  msg2 = _mm_add_epi32(msg2, tmp);
  msg2 = _mm_sha256msg2_epu32(msg2, msg1);
  msg = _mm_shuffle_epi32(msg, 0x0E);
  state0 = _mm_sha256rnds2_epu32(state0, state1, msg);
  msg0 = _mm_sha256msg1_epu32(msg0, msg1);

  /* Rounds 24-27 */
  msg = _mm_add_epi32(
      msg2,
      _mm_set_epi64x(0xBF597FC7B00327C8ULL, 0xA831C66D983E5152ULL));
  state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
  tmp = _mm_alignr_epi8(msg2, msg1, 4);
  msg3 = _mm_add_epi32(msg3, tmp);
  msg3 = _mm_sha256msg2_epu32(msg3, msg2);
  msg = _mm_shuffle_epi32(msg, 0x0E);
  state0 = _mm_sha256rnds2_epu32(state0, state1, msg);
  msg1 = _mm_sha256msg1_epu32(msg1, msg2);

  /* Rounds 28-31 */
  msg = _mm_add_epi32(
      msg3,
      _mm_set_epi64x(0x1429296706CA6351ULL, 0xD5A79147C6E00BF3ULL));
  state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
  tmp = _mm_alignr_epi8(msg3, msg2, 4);
  msg0 = _mm_add_epi32(msg0, tmp);
  msg0 = _mm_sha256msg2_epu32(msg0, msg3);
  msg = _mm_shuffle_epi32(msg, 0x0E);
  state0 = _mm_sha256rnds2_epu32(state0, state1, msg);
  msg2 = _mm_sha256msg1_epu32(msg2, msg3);

  /* Rounds 32-35 */
  msg = _mm_add_epi32(
      msg0,
      _mm_set_epi64x(0x53380D134D2C6DFCULL, 0x2E1B213827B70A85ULL));
  state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
  tmp = _mm_alignr_epi8(msg0, msg3, 4);
  msg1 = _mm_add_epi32(msg1, tmp);
  msg1 = _mm_sha256msg2_epu32(msg1, msg0);
  msg = _mm_shuffle_epi32(msg, 0x0E);
  state0 = _mm_sha256rnds2_epu32(state0, state1, msg);
  msg3 = _mm_sha256msg1_epu32(msg3, msg0);

  /* Rounds 36-39 */
  msg = _mm_add_epi32(
      msg1,
      _mm_set_epi64x(0x92722C8581C2C92EULL, 0x766A0ABB650A7354ULL));
  state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
  tmp = _mm_alignr_epi8(msg1, msg0, 4);
  msg2 = _mm_add_epi32(msg2, tmp);
  msg2 = _mm_sha256msg2_epu32(msg2, msg1);
  msg = _mm_shuffle_epi32(msg, 0x0E);
  state0 = _mm_sha256rnds2_epu32(state0, state1, msg);
  msg0 = _mm_sha256msg1_epu32(msg0, msg1);

  /* Rounds 40-43 */
  msg = _mm_add_epi32(
      msg2,
      _mm_set_epi64x(0xC76C51A3C24B8B70ULL, 0xA81A664BA2BFE8A1ULL));
  state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
  tmp = _mm_alignr_epi8(msg2, msg1, 4);
  msg3 = _mm_add_epi32(msg3, tmp);
  msg3 = _mm_sha256msg2_epu32(msg3, msg2);
  msg = _mm_shuffle_epi32(msg, 0x0E);
  state0 = _mm_sha256rnds2_epu32(state0, state1, msg);
  msg1 = _mm_sha256msg1_epu32(msg1, msg2);

  /* Rounds 44-47 */
  msg = _mm_add_epi32(
      msg3,
      _mm_set_epi64x(0x106AA070F40E3585ULL, 0xD6990624D192E819ULL));
  state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
  tmp = _mm_alignr_epi8(msg3, msg2, 4);
  msg0 = _mm_add_epi32(msg0, tmp);
  msg0 = _mm_sha256msg2_epu32(msg0, msg3);
  msg = _mm_shuffle_epi32(msg, 0x0E);
  state0 = _mm_sha256rnds2_epu32(state0, state1, msg);
  msg2 = _mm_sha256msg1_epu32(msg2, msg3);

  /* Rounds 48-51 */
  msg = _mm_add_epi32(
      msg0,
      _mm_set_epi64x(0x34B0BCB52748774CULL, 0x1E376C0819A4C116ULL));
  state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
  tmp = _mm_alignr_epi8(msg0, msg3, 4);
  msg1 = _mm_add_epi32(msg1, tmp);
  msg1 = _mm_sha256msg2_epu32(msg1, msg0);
  msg = _mm_shuffle_epi32(msg, 0x0E);
  state0 = _mm_sha256rnds2_epu32(state0, state1, msg);
  msg3 = _mm_sha256msg1_epu32(msg3, msg0);

  /* Rounds 52-55 */
  msg = _mm_add_epi32(
      msg1,
      _mm_set_epi64x(0x5B9CCA4F4ED8AA4AULL, 0x391C0CB3391C0CB3ULL));
  state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
  tmp = _mm_alignr_epi8(msg1, msg0, 4);
  msg2 = _mm_add_epi32(msg2, tmp);
  msg2 = _mm_sha256msg2_epu32(msg2, msg1);
  msg = _mm_shuffle_epi32(msg, 0x0E);
  state0 = _mm_sha256rnds2_epu32(state0, state1, msg);

  /* Rounds 56-59 */
  msg = _mm_add_epi32(
      msg2,
      _mm_set_epi64x(0x8CC7020884C87814ULL, 0x78A5636F748F82EEULL));
  state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
  tmp = _mm_alignr_epi8(msg2, msg1, 4);
  msg3 = _mm_add_epi32(msg3, tmp);
  msg3 = _mm_sha256msg2_epu32(msg3, msg2);
  msg = _mm_shuffle_epi32(msg, 0x0E);
  state0 = _mm_sha256rnds2_epu32(state0, state1, msg);

  /* Rounds 60-63 */
  msg = _mm_add_epi32(
      msg3,
      _mm_set_epi64x(0xC67178F2BEF9A3F7ULL, 0xA4506CEB90BEFFFAULL));
  state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
  msg = _mm_shuffle_epi32(msg, 0x0E);
  state0 = _mm_sha256rnds2_epu32(state0, state1, msg);

  /* Combine state */
  state0 = _mm_add_epi32(state0, abef_save);
  state1 = _mm_add_epi32(state1, cdgh_save);

  tmp = _mm_shuffle_epi32(state0, 0x1B);       /* FEBA */
  state1 = _mm_shuffle_epi32(state1, 0xB1);    /* DCHG */
  state0 = _mm_blend_epi16(tmp, state1, 0xF0); /* DCBA */
  state1 = _mm_alignr_epi8(state1, tmp, 8);    /* ABEF */

  /* Save state */
  _mm_storeu_si128((__m128i *)&h->u32[0], state0);
  _mm_storeu_si128((__m128i *)&h->u32[4], state1);

#elif FIO___HAS_ARM_INTRIN
  /* Code adjusted from:
   * https://github.com/noloader/SHA-Intrinsics/blob/master/sha256-arm.c
   * Credit to Jeffrey Walton.
   */
  static const uint32_t sha256_consts[64] FIO_ALIGN(16) = {
      0x428A2F98UL, 0x71374491UL, 0xB5C0FBCFUL, 0xE9B5DBA5UL, 0x3956C25BUL,
      0x59F111F1UL, 0x923F82A4UL, 0xAB1C5ED5UL, 0xD807AA98UL, 0x12835B01UL,
      0x243185BEUL, 0x550C7DC3UL, 0x72BE5D74UL, 0x80DEB1FEUL, 0x9BDC06A7UL,
      0xC19BF174UL, 0xE49B69C1UL, 0xEFBE4786UL, 0x0FC19DC6UL, 0x240CA1CCUL,
      0x2DE92C6FUL, 0x4A7484AAUL, 0x5CB0A9DCUL, 0x76F988DAUL, 0x983E5152UL,
      0xA831C66DUL, 0xB00327C8UL, 0xBF597FC7UL, 0xC6E00BF3UL, 0xD5A79147UL,
      0x06CA6351UL, 0x14292967UL, 0x27B70A85UL, 0x2E1B2138UL, 0x4D2C6DFCUL,
      0x53380D13UL, 0x650A7354UL, 0x766A0ABBUL, 0x81C2C92EUL, 0x92722C85UL,
      0xA2BFE8A1UL, 0xA81A664BUL, 0xC24B8B70UL, 0xC76C51A3UL, 0xD192E819UL,
      0xD6990624UL, 0xF40E3585UL, 0x106AA070UL, 0x19A4C116UL, 0x1E376C08UL,
      0x2748774CUL, 0x34B0BCB5UL, 0x391C0CB3UL, 0x4ED8AA4AUL, 0x5B9CCA4FUL,
      0x682E6FF3UL, 0x748F82EEUL, 0x78A5636FUL, 0x84C87814UL, 0x8CC70208UL,
      0x90BEFFFAUL, 0xA4506CEBUL, 0xBEF9A3F7UL, 0xC67178F2UL};

  uint32x4_t state0, state1, state0_save, state1_save;
  uint32x4_t msg0, msg1, msg2, msg3;
  uint32x4_t tmp0, tmp1, tmp2;

  /* Load state */
  state0 = vld1q_u32(&h->u32[0]);
  state1 = vld1q_u32(&h->u32[4]);

  /* Save state */
  state0_save = state0;
  state1_save = state1;

  /* Load and byte-swap message */
  msg0 = vld1q_u32((const uint32_t *)(block + 0));
  msg1 = vld1q_u32((const uint32_t *)(block + 16));
  msg2 = vld1q_u32((const uint32_t *)(block + 32));
  msg3 = vld1q_u32((const uint32_t *)(block + 48));
  msg0 = vreinterpretq_u32_u8(vrev32q_u8(vreinterpretq_u8_u32(msg0)));
  msg1 = vreinterpretq_u32_u8(vrev32q_u8(vreinterpretq_u8_u32(msg1)));
  msg2 = vreinterpretq_u32_u8(vrev32q_u8(vreinterpretq_u8_u32(msg2)));
  msg3 = vreinterpretq_u32_u8(vrev32q_u8(vreinterpretq_u8_u32(msg3)));

  /* Rounds 0-3 */
  tmp0 = vaddq_u32(msg0, vld1q_u32(&sha256_consts[0]));
  tmp2 = state0;
  state0 = vsha256hq_u32(state0, state1, tmp0);
  state1 = vsha256h2q_u32(state1, tmp2, tmp0);
  msg0 = vsha256su0q_u32(msg0, msg1);

  /* Rounds 4-7 */
  tmp1 = vaddq_u32(msg1, vld1q_u32(&sha256_consts[4]));
  tmp2 = state0;
  state0 = vsha256hq_u32(state0, state1, tmp1);
  state1 = vsha256h2q_u32(state1, tmp2, tmp1);
  msg0 = vsha256su1q_u32(msg0, msg2, msg3);
  msg1 = vsha256su0q_u32(msg1, msg2);

  /* Rounds 8-11 */
  tmp0 = vaddq_u32(msg2, vld1q_u32(&sha256_consts[8]));
  tmp2 = state0;
  state0 = vsha256hq_u32(state0, state1, tmp0);
  state1 = vsha256h2q_u32(state1, tmp2, tmp0);
  msg1 = vsha256su1q_u32(msg1, msg3, msg0);
  msg2 = vsha256su0q_u32(msg2, msg3);

  /* Rounds 12-15 */
  tmp1 = vaddq_u32(msg3, vld1q_u32(&sha256_consts[12]));
  tmp2 = state0;
  state0 = vsha256hq_u32(state0, state1, tmp1);
  state1 = vsha256h2q_u32(state1, tmp2, tmp1);
  msg2 = vsha256su1q_u32(msg2, msg0, msg1);
  msg3 = vsha256su0q_u32(msg3, msg0);

  /* Rounds 16-19 */
  tmp0 = vaddq_u32(msg0, vld1q_u32(&sha256_consts[16]));
  tmp2 = state0;
  state0 = vsha256hq_u32(state0, state1, tmp0);
  state1 = vsha256h2q_u32(state1, tmp2, tmp0);
  msg3 = vsha256su1q_u32(msg3, msg1, msg2);
  msg0 = vsha256su0q_u32(msg0, msg1);

  /* Rounds 20-23 */
  tmp1 = vaddq_u32(msg1, vld1q_u32(&sha256_consts[20]));
  tmp2 = state0;
  state0 = vsha256hq_u32(state0, state1, tmp1);
  state1 = vsha256h2q_u32(state1, tmp2, tmp1);
  msg0 = vsha256su1q_u32(msg0, msg2, msg3);
  msg1 = vsha256su0q_u32(msg1, msg2);

  /* Rounds 24-27 */
  tmp0 = vaddq_u32(msg2, vld1q_u32(&sha256_consts[24]));
  tmp2 = state0;
  state0 = vsha256hq_u32(state0, state1, tmp0);
  state1 = vsha256h2q_u32(state1, tmp2, tmp0);
  msg1 = vsha256su1q_u32(msg1, msg3, msg0);
  msg2 = vsha256su0q_u32(msg2, msg3);

  /* Rounds 28-31 */
  tmp1 = vaddq_u32(msg3, vld1q_u32(&sha256_consts[28]));
  tmp2 = state0;
  state0 = vsha256hq_u32(state0, state1, tmp1);
  state1 = vsha256h2q_u32(state1, tmp2, tmp1);
  msg2 = vsha256su1q_u32(msg2, msg0, msg1);
  msg3 = vsha256su0q_u32(msg3, msg0);

  /* Rounds 32-35 */
  tmp0 = vaddq_u32(msg0, vld1q_u32(&sha256_consts[32]));
  tmp2 = state0;
  state0 = vsha256hq_u32(state0, state1, tmp0);
  state1 = vsha256h2q_u32(state1, tmp2, tmp0);
  msg3 = vsha256su1q_u32(msg3, msg1, msg2);
  msg0 = vsha256su0q_u32(msg0, msg1);

  /* Rounds 36-39 */
  tmp1 = vaddq_u32(msg1, vld1q_u32(&sha256_consts[36]));
  tmp2 = state0;
  state0 = vsha256hq_u32(state0, state1, tmp1);
  state1 = vsha256h2q_u32(state1, tmp2, tmp1);
  msg0 = vsha256su1q_u32(msg0, msg2, msg3);
  msg1 = vsha256su0q_u32(msg1, msg2);

  /* Rounds 40-43 */
  tmp0 = vaddq_u32(msg2, vld1q_u32(&sha256_consts[40]));
  tmp2 = state0;
  state0 = vsha256hq_u32(state0, state1, tmp0);
  state1 = vsha256h2q_u32(state1, tmp2, tmp0);
  msg1 = vsha256su1q_u32(msg1, msg3, msg0);
  msg2 = vsha256su0q_u32(msg2, msg3);

  /* Rounds 44-47 */
  tmp1 = vaddq_u32(msg3, vld1q_u32(&sha256_consts[44]));
  tmp2 = state0;
  state0 = vsha256hq_u32(state0, state1, tmp1);
  state1 = vsha256h2q_u32(state1, tmp2, tmp1);
  msg2 = vsha256su1q_u32(msg2, msg0, msg1);
  msg3 = vsha256su0q_u32(msg3, msg0);

  /* Rounds 48-51 */
  tmp0 = vaddq_u32(msg0, vld1q_u32(&sha256_consts[48]));
  tmp2 = state0;
  state0 = vsha256hq_u32(state0, state1, tmp0);
  state1 = vsha256h2q_u32(state1, tmp2, tmp0);
  msg3 = vsha256su1q_u32(msg3, msg1, msg2);

  /* Rounds 52-55 */
  tmp1 = vaddq_u32(msg1, vld1q_u32(&sha256_consts[52]));
  tmp2 = state0;
  state0 = vsha256hq_u32(state0, state1, tmp1);
  state1 = vsha256h2q_u32(state1, tmp2, tmp1);

  /* Rounds 56-59 */
  tmp0 = vaddq_u32(msg2, vld1q_u32(&sha256_consts[56]));
  tmp2 = state0;
  state0 = vsha256hq_u32(state0, state1, tmp0);
  state1 = vsha256h2q_u32(state1, tmp2, tmp0);

  /* Rounds 60-63 */
  tmp1 = vaddq_u32(msg3, vld1q_u32(&sha256_consts[60]));
  tmp2 = state0;
  state0 = vsha256hq_u32(state0, state1, tmp1);
  state1 = vsha256h2q_u32(state1, tmp2, tmp1);

  /* Combine state */
  state0 = vaddq_u32(state0, state0_save);
  state1 = vaddq_u32(state1, state1_save);

  /* Save state */
  vst1q_u32(&h->u32[0], state0);
  vst1q_u32(&h->u32[4], state1);

#else /* Portable implementation */
  static const uint32_t sha256_consts[64] = {
      0x428A2F98UL, 0x71374491UL, 0xB5C0FBCFUL, 0xE9B5DBA5UL, 0x3956C25BUL,
      0x59F111F1UL, 0x923F82A4UL, 0xAB1C5ED5UL, 0xD807AA98UL, 0x12835B01UL,
      0x243185BEUL, 0x550C7DC3UL, 0x72BE5D74UL, 0x80DEB1FEUL, 0x9BDC06A7UL,
      0xC19BF174UL, 0xE49B69C1UL, 0xEFBE4786UL, 0x0FC19DC6UL, 0x240CA1CCUL,
      0x2DE92C6FUL, 0x4A7484AAUL, 0x5CB0A9DCUL, 0x76F988DAUL, 0x983E5152UL,
      0xA831C66DUL, 0xB00327C8UL, 0xBF597FC7UL, 0xC6E00BF3UL, 0xD5A79147UL,
      0x06CA6351UL, 0x14292967UL, 0x27B70A85UL, 0x2E1B2138UL, 0x4D2C6DFCUL,
      0x53380D13UL, 0x650A7354UL, 0x766A0ABBUL, 0x81C2C92EUL, 0x92722C85UL,
      0xA2BFE8A1UL, 0xA81A664BUL, 0xC24B8B70UL, 0xC76C51A3UL, 0xD192E819UL,
      0xD6990624UL, 0xF40E3585UL, 0x106AA070UL, 0x19A4C116UL, 0x1E376C08UL,
      0x2748774CUL, 0x34B0BCB5UL, 0x391C0CB3UL, 0x4ED8AA4AUL, 0x5B9CCA4FUL,
      0x682E6FF3UL, 0x748F82EEUL, 0x78A5636FUL, 0x84C87814UL, 0x8CC70208UL,
      0x90BEFFFAUL, 0xA4506CEBUL, 0xBEF9A3F7UL, 0xC67178F2UL};

  uint32_t v[8];
  for (size_t i = 0; i < 8; ++i)
    v[i] = h->u32[i];

  /* read data as an array of 16 big endian 32 bit integers. */
  uint32_t w[16] FIO_ALIGN(16);
  fio_memcpy64(w, block);
  for (size_t i = 0; i < 16; ++i)
    w[i] = fio_lton32(w[i]); /* no-op on big endian systems */

    /* SHA-256 round - processes one word */
#define FIO___SHA256_ROUND(i, k)                                               \
  do {                                                                         \
    const uint32_t t1 = v[7] + (k) + w[(i)&15] +                               \
                        fio_ct_mux32(v[4], v[5], v[6]) +                       \
                        fio_xor_rrot3_32(v[4], 6, 11, 25);                     \
    const uint32_t t2 =                                                        \
        fio_ct_maj32(v[0], v[1], v[2]) + fio_xor_rrot3_32(v[0], 2, 13, 22);    \
    v[7] = v[6];                                                               \
    v[6] = v[5];                                                               \
    v[5] = v[4];                                                               \
    v[4] = v[3] + t1;                                                          \
    v[3] = v[2];                                                               \
    v[2] = v[1];                                                               \
    v[1] = v[0];                                                               \
    v[0] = t1 + t2;                                                            \
  } while (0)

  /* First 16 rounds - use message words directly */
  for (size_t i = 0; i < 16; ++i)
    FIO___SHA256_ROUND(i, sha256_consts[i]);

  /* Remaining 48 rounds - expand message schedule inline */
  for (size_t i = 16; i < 64; ++i) {
    w[i & 15] = fio_xor_rrot2_shr_32(w[(i + 14) & 15], 17, 19, 10) +
                w[(i + 9) & 15] +
                fio_xor_rrot2_shr_32(w[(i + 1) & 15], 7, 18, 3) + w[i & 15];
    FIO___SHA256_ROUND(i, sha256_consts[i]);
  }
#undef FIO___SHA256_ROUND

  /* Add compressed chunk to current hash value */
  for (size_t i = 0; i < 8; ++i)
    h->u32[i] += v[i];

#endif /* FIO___HAS_X86_SHA_INTRIN / FIO___HAS_ARM_INTRIN */
}

/** consume data and feed it to hash. */
SFUNC void fio_sha256_consume(fio_sha256_s *h, const void *data, uint64_t len) {
  const uint8_t *r = (const uint8_t *)data;
  const size_t old_total = h->total_len;
  const size_t new_total = len + h->total_len;
  h->total_len = new_total;
  /* manage cache */
  if (old_total & 63) {
    const size_t offset = (old_total & 63);
    if (len + offset < 64) { /* not enough - copy to cache */
      fio_memcpy63x((h->cache.u8 + offset), r, len);
      return;
    }
    /* consume cache */
    const size_t byte2copy = 64UL - offset;
    fio_memcpy63x(h->cache.u8 + offset, r, byte2copy);
    fio___sha256_round(&h->hash, h->cache.u8);
    FIO_MEMSET(h->cache.u8, 0, 64);
    r += byte2copy;
    len -= byte2copy;
  }
  const uint8_t *end = r + (len & (~(uint64_t)63ULL));
  while ((uintptr_t)r < (uintptr_t)end) {
    fio___sha256_round(&h->hash, r);
    r += 64;
  }
  fio_memcpy63x(h->cache.u64, r, len);
}

SFUNC fio_u256 fio_sha256_finalize(fio_sha256_s *h) {
  if (h->total_len == ((uint64_t)0ULL - 1ULL))
    return h->hash;
  const size_t total = h->total_len;
  size_t remainder = total & 63;
  h->cache.u8[remainder] = 0x80U; /* set the 1 bit at the left most position */
  if ((remainder) > 55) { /* make sure there's room to attach `total_len` */
    fio___sha256_round(&h->hash, h->cache.u8);
    FIO_MEMSET(h->cache.u8, 0, 64);
  }
  h->cache.u64[7] = fio_lton64((total << 3));
  fio___sha256_round(&h->hash, h->cache.u8);
  for (size_t i = 0; i < 8; ++i)
    h->hash.u32[i] = fio_ntol32(h->hash.u32[i]); /* back to big endien */
  h->total_len = ((uint64_t)0ULL - 1ULL);
  return h->hash;
}

/* *****************************************************************************
Implementation - SHA-512
***************************************************************************** */

FIO_IFUNC void fio___sha512_round(fio_u512 *h, const uint8_t *block) {
  static const uint64_t sha512_consts[80] = {
      0x428A2F98D728AE22ULL, 0x7137449123EF65CDULL, 0xB5C0FBCFEC4D3B2FULL,
      0xE9B5DBA58189DBBCULL, 0x3956C25BF348B538ULL, 0x59F111F1B605D019ULL,
      0x923F82A4AF194F9BULL, 0xAB1C5ED5DA6D8118ULL, 0xD807AA98A3030242ULL,
      0x12835B0145706FBEULL, 0x243185BE4EE4B28CULL, 0x550C7DC3D5FFB4E2ULL,
      0x72BE5D74F27B896FULL, 0x80DEB1FE3B1696B1ULL, 0x9BDC06A725C71235ULL,
      0xC19BF174CF692694ULL, 0xE49B69C19EF14AD2ULL, 0xEFBE4786384F25E3ULL,
      0x0FC19DC68B8CD5B5ULL, 0x240CA1CC77AC9C65ULL, 0x2DE92C6F592B0275ULL,
      0x4A7484AA6EA6E483ULL, 0x5CB0A9DCBD41FBD4ULL, 0x76F988DA831153B5ULL,
      0x983E5152EE66DFABULL, 0xA831C66D2DB43210ULL, 0xB00327C898FB213FULL,
      0xBF597FC7BEEF0EE4ULL, 0xC6E00BF33DA88FC2ULL, 0xD5A79147930AA725ULL,
      0x06CA6351E003826FULL, 0x142929670A0E6E70ULL, 0x27B70A8546D22FFCULL,
      0x2E1B21385C26C926ULL, 0x4D2C6DFC5AC42AEDULL, 0x53380D139D95B3DFULL,
      0x650A73548BAF63DEULL, 0x766A0ABB3C77B2A8ULL, 0x81C2C92E47EDAEE6ULL,
      0x92722C851482353BULL, 0xA2BFE8A14CF10364ULL, 0xA81A664BBC423001ULL,
      0xC24B8B70D0F89791ULL, 0xC76C51A30654BE30ULL, 0xD192E819D6EF5218ULL,
      0xD69906245565A910ULL, 0xF40E35855771202AULL, 0x106AA07032BBD1B8ULL,
      0x19A4C116B8D2D0C8ULL, 0x1E376C085141AB53ULL, 0x2748774CDF8EEB99ULL,
      0x34B0BCB5E19B48A8ULL, 0x391C0CB3C5C95A63ULL, 0x4ED8AA4AE3418ACBULL,
      0x5B9CCA4F7763E373ULL, 0x682E6FF3D6B2B8A3ULL, 0x748F82EE5DEFB2FCULL,
      0x78A5636F43172F60ULL, 0x84C87814A1F0AB72ULL, 0x8CC702081A6439ECULL,
      0x90BEFFFA23631E28ULL, 0xA4506CEBDE82BDE9ULL, 0xBEF9A3F7B2C67915ULL,
      0xC67178F2E372532BULL, 0xCA273ECEEA26619CULL, 0xD186B8C721C0C207ULL,
      0xEADA7DD6CDE0EB1EULL, 0xF57D4F7FEE6ED178ULL, 0x06F067AA72176FBAULL,
      0x0A637DC5A2C898A6ULL, 0x113F9804BEF90DAEULL, 0x1B710B35131C471BULL,
      0x28DB77F523047D84ULL, 0x32CAAB7B40C72493ULL, 0x3C9EBE0A15C9BEBCULL,
      0x431D67C49C100D4CULL, 0x4CC5D4BECB3E42B6ULL, 0x597F299CFC657E2AULL,
      0x5FCB6FAB3AD6FAECULL, 0x6C44198C4A475817ULL};

  /* copy original state */
  uint64_t v[8] FIO_ALIGN(16);
  for (size_t i = 0; i < 8; ++i)
    v[i] = h->u64[i];

  /* read data as an array of 16 big endian 64 bit integers. */
  uint64_t w[16] FIO_ALIGN(16);
  fio_memcpy128(w, block);
  for (size_t i = 0; i < 16; ++i)
    w[i] = fio_lton64(w[i]); /* no-op on big endian systems */

    /* SHA-512 round - processes one word */
#define FIO___SHA512_ROUND(i, k)                                               \
  do {                                                                         \
    const uint64_t t1 = v[7] + (k) + w[(i)&15] +                               \
                        fio_ct_mux64(v[4], v[5], v[6]) +                       \
                        fio_xor_rrot3_64(v[4], 14, 18, 41);                    \
    const uint64_t t2 =                                                        \
        fio_ct_maj64(v[0], v[1], v[2]) + fio_xor_rrot3_64(v[0], 28, 34, 39);   \
    v[7] = v[6];                                                               \
    v[6] = v[5];                                                               \
    v[5] = v[4];                                                               \
    v[4] = v[3] + t1;                                                          \
    v[3] = v[2];                                                               \
    v[2] = v[1];                                                               \
    v[1] = v[0];                                                               \
    v[0] = t1 + t2;                                                            \
  } while (0)

  /* First 16 rounds - use message words directly */
  for (size_t i = 0; i < 16; ++i)
    FIO___SHA512_ROUND(i, sha512_consts[i]);

  /* Remaining 64 rounds - expand message schedule inline */
  for (size_t i = 16; i < 80; ++i) {
    w[i & 15] = fio_xor_rrot2_shr_64(w[(i + 14) & 15], 19, 61, 6) +
                w[(i + 9) & 15] +
                fio_xor_rrot2_shr_64(w[(i + 1) & 15], 1, 8, 7) + w[i & 15];
    FIO___SHA512_ROUND(i, sha512_consts[i]);
  }
#undef FIO___SHA512_ROUND

  /* Add compressed chunk to current hash value */
  for (size_t i = 0; i < 8; ++i)
    h->u64[i] += v[i];
}

/** Feed data into the hash */
SFUNC void fio_sha512_consume(fio_sha512_s *restrict h,
                              const void *restrict data,
                              uint64_t len) {
  const uint8_t *r = (const uint8_t *)data;
  const size_t old_total = h->total_len;
  const size_t new_total = len + h->total_len;
  h->total_len = new_total;
  /* manage cache */
  if (old_total & 127) {
    const size_t offset = (old_total & 127);
    if (len + offset < 128) { /* not enough - copy to cache */
      fio_memcpy127x((h->cache.u8 + offset), r, len);
      return;
    }
    /* consume cache */
    const size_t byte2copy = 128UL - offset;
    fio_memcpy127x(h->cache.u8 + offset, r, byte2copy);
    fio___sha512_round(&h->hash, h->cache.u8);
    h->cache = (fio_u1024){0};
    r += byte2copy;
    len -= byte2copy;
  }
  const uint8_t *end = r + (len & (~(uint64_t)127ULL));
  while ((uintptr_t)r < (uintptr_t)end) {
    fio___sha512_round(&h->hash, r);
    r += 128;
  }
  fio_memcpy127x(h->cache.u64, r, len);
}

/** finalizes a fio_u512 with the SHA 512 hash. */
SFUNC fio_u512 fio_sha512_finalize(fio_sha512_s *h) {
  if (h->total_len == ((uint64_t)0ULL - 1ULL))
    return h->hash;
  const size_t total = h->total_len;
  const size_t remainder = total & 127;
  h->cache.u8[remainder] = 0x80U; /* set the 1 bit at the left most position */
  if ((remainder) > 112) { /* make sure there's room to attach `total_len` */
    fio___sha512_round(&h->hash, h->cache.u8);
    h->cache = (fio_u1024){0};
  }
  h->cache.u64[15] = fio_lton64((total << 3));
  fio___sha512_round(&h->hash, h->cache.u8);
  for (size_t i = 0; i < 8; ++i)
    h->hash.u64[i] = fio_ntol64(h->hash.u64[i]); /* back to/from big endien */
  h->total_len = ((uint64_t)0ULL - 1ULL);
  return h->hash;
}

/* *****************************************************************************
HMAC
***************************************************************************** */

/**
 * HMAC-SHA256, resulting in a 32 byte authentication code.
 *
 * Keys are limited to 64 bytes due to the design of the HMAC algorithm.
 */
SFUNC fio_u256 fio_sha256_hmac(const void *key,
                               uint64_t key_len,
                               const void *msg,
                               uint64_t msg_len) {
  fio_u512 k = {0};
  /* copy key - SHA-256 block size is 64 bytes */
  if (key_len > 64) {
    /* keys longer than block size are hashed first */
    k.u256[0] = fio_sha256(key, key_len);
    key_len = 32;
  } else {
    FIO_MEMCPY(k.u8, key, key_len);
  }
  /* prepare inner key (k XOR ipad) */
  for (size_t i = 0; i < 8; ++i)
    k.u64[i] ^= (uint64_t)0x3636363636363636ULL;
  /* inner hash = H((k XOR ipad) || msg) */
  fio_sha256_s inner = fio_sha256_init();
  fio_sha256_consume(&inner, k.u8, 64);
  fio_sha256_consume(&inner, msg, msg_len);
  fio_u256 inner_hash = fio_sha256_finalize(&inner);
  /* switch to outer key (k XOR opad) */
  for (size_t i = 0; i < 8; ++i)
    k.u64[i] ^=
        ((uint64_t)0x3636363636363636ULL ^ (uint64_t)0x5C5C5C5C5C5C5C5CULL);
  /* outer hash = H((k XOR opad) || inner_hash) */
  fio_sha256_s outer = fio_sha256_init();
  fio_sha256_consume(&outer, k.u8, 64);
  fio_sha256_consume(&outer, inner_hash.u8, 32);
  return fio_sha256_finalize(&outer);
}
/**
 * HMAC-SHA512, resulting in a 64 byte authentication code.
 *
 * Keys are limited to 128 bytes due to the design of the HMAC algorithm.
 */
SFUNC fio_u512 fio_sha512_hmac(const void *key,
                               uint64_t key_len,
                               const void *msg,
                               uint64_t msg_len) {
  fio_sha512_s inner = fio_sha512_init();
  fio_u2048 k;
  /* copy key */
  if (key_len > 128)
    goto key_too_long;
  if (key_len == 128)
    fio_memcpy128(k.u8, key);
  else {
    k.u1024[0] = (fio_u1024){0};
    fio_memcpy127x(k.u8, key, key_len);
  }
  /* prepare inner key */
  for (size_t i = 0; i < 16; ++i)
    k.u64[i] ^= (uint64_t)0x3636363636363636ULL;
  /* hash of inner key + msg
   * It's the same as the following, but easier for compilers to optimize:
   * fio_sha512_consume(&inner, k.u8, 128);
   * fio_sha512_consume(&inner, msg, msg_len); */
  {
    /* consume key block */
    fio___sha512_round(&inner.hash, k.u8);
    /* consume data */
    uint8_t *buf = (uint8_t *)msg;
    for (size_t i = 127; i < msg_len; (i += 128), (buf += 128))
      fio___sha512_round(&inner.hash, buf);
    if ((msg_len & 127)) {
      inner.cache = (fio_u1024){0};
      fio_memcpy127x(inner.cache.u8, buf, msg_len);
    }
    inner.total_len = 128 + msg_len;
  }
  /* finalize SHA512 and append to end of key */
  k.u512[2] = fio_sha512_finalize(&inner);
  /* switch key to outer key */
  for (size_t i = 0; i < 16; ++i)
    k.u64[i] ^=
        ((uint64_t)0x3636363636363636ULL ^ (uint64_t)0x5C5C5C5C5C5C5C5CULL);
  /* hash outer key with inner hash appended and return */
  return fio_sha512(k.u8, 192);

key_too_long:
  k.u512[3] = fio_sha512(key, key_len);
  return fio_sha512_hmac(k.u512[3].u8, sizeof(k.u512[3]), msg, msg_len);
}

/* *****************************************************************************
Cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_SHA2 */
#undef FIO_SHA2
