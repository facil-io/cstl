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
#if defined(FIO___HAS_X86_SHA_INTRIN) && FIO___HAS_X86_SHA_INTRIN
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
  if (!len)
    return;
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

FIO_IFUNC void fio___sha512_round(fio_u512 *restrict h,
                                  const uint8_t *restrict block) {
#if defined(FIO___HAS_ARM_SHA512_INTRIN) && FIO___HAS_ARM_SHA512_INTRIN
  /* ARM SHA512 intrinsics implementation (ARMv8.4-A+)
   * Based on Simon Tatham's implementation for PuTTY (MIT/Apache 2.0)
   * Uses vsha512hq_u64, vsha512h2q_u64, vsha512su0q_u64, vsha512su1q_u64
   */
  static const uint64_t K[80] FIO_ALIGN(16) = {
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

  /* SHA-512 state: 4 vectors of 2x64-bit each = 512 bits total
   * State layout: ab={a,b}, cd={c,d}, ef={e,f}, gh={g,h}
   */
  uint64x2_t ab, cd, ef, gh;
  uint64x2_t ab_orig, cd_orig, ef_orig, gh_orig;
  /* Message schedule: 8 vectors of 2x64-bit each = 16 words */
  uint64x2_t s0, s1, s2, s3, s4, s5, s6, s7;
  uint64x2_t initial_sum, sum, intermed;

  /* Load state */
  ab = vld1q_u64(&h->u64[0]);
  cd = vld1q_u64(&h->u64[2]);
  ef = vld1q_u64(&h->u64[4]);
  gh = vld1q_u64(&h->u64[6]);

  /* Save state for final addition */
  ab_orig = ab;
  cd_orig = cd;
  ef_orig = ef;
  gh_orig = gh;

  /* Load and byte-swap message (SHA-512 uses big-endian) */
  s0 = vreinterpretq_u64_u8(
      vrev64q_u8(vreinterpretq_u8_u64(vld1q_u64((const uint64_t *)block))));
  s1 = vreinterpretq_u64_u8(vrev64q_u8(
      vreinterpretq_u8_u64(vld1q_u64((const uint64_t *)(block + 16)))));
  s2 = vreinterpretq_u64_u8(vrev64q_u8(
      vreinterpretq_u8_u64(vld1q_u64((const uint64_t *)(block + 32)))));
  s3 = vreinterpretq_u64_u8(vrev64q_u8(
      vreinterpretq_u8_u64(vld1q_u64((const uint64_t *)(block + 48)))));
  s4 = vreinterpretq_u64_u8(vrev64q_u8(
      vreinterpretq_u8_u64(vld1q_u64((const uint64_t *)(block + 64)))));
  s5 = vreinterpretq_u64_u8(vrev64q_u8(
      vreinterpretq_u8_u64(vld1q_u64((const uint64_t *)(block + 80)))));
  s6 = vreinterpretq_u64_u8(vrev64q_u8(
      vreinterpretq_u8_u64(vld1q_u64((const uint64_t *)(block + 96)))));
  s7 = vreinterpretq_u64_u8(vrev64q_u8(
      vreinterpretq_u8_u64(vld1q_u64((const uint64_t *)(block + 112)))));

  /* Rounds 0 and 1 */
  initial_sum = vaddq_u64(s0, vld1q_u64(&K[0]));
  sum = vaddq_u64(vextq_u64(initial_sum, initial_sum, 1), gh);
  intermed = vsha512hq_u64(sum, vextq_u64(ef, gh, 1), vextq_u64(cd, ef, 1));
  gh = vsha512h2q_u64(intermed, cd, ab);
  cd = vaddq_u64(cd, intermed);

  /* Rounds 2 and 3 */
  initial_sum = vaddq_u64(s1, vld1q_u64(&K[2]));
  sum = vaddq_u64(vextq_u64(initial_sum, initial_sum, 1), ef);
  intermed = vsha512hq_u64(sum, vextq_u64(cd, ef, 1), vextq_u64(ab, cd, 1));
  ef = vsha512h2q_u64(intermed, ab, gh);
  ab = vaddq_u64(ab, intermed);

  /* Rounds 4 and 5 */
  initial_sum = vaddq_u64(s2, vld1q_u64(&K[4]));
  sum = vaddq_u64(vextq_u64(initial_sum, initial_sum, 1), cd);
  intermed = vsha512hq_u64(sum, vextq_u64(ab, cd, 1), vextq_u64(gh, ab, 1));
  cd = vsha512h2q_u64(intermed, gh, ef);
  gh = vaddq_u64(gh, intermed);

  /* Rounds 6 and 7 */
  initial_sum = vaddq_u64(s3, vld1q_u64(&K[6]));
  sum = vaddq_u64(vextq_u64(initial_sum, initial_sum, 1), ab);
  intermed = vsha512hq_u64(sum, vextq_u64(gh, ab, 1), vextq_u64(ef, gh, 1));
  ab = vsha512h2q_u64(intermed, ef, cd);
  ef = vaddq_u64(ef, intermed);

  /* Rounds 8 and 9 */
  initial_sum = vaddq_u64(s4, vld1q_u64(&K[8]));
  sum = vaddq_u64(vextq_u64(initial_sum, initial_sum, 1), gh);
  intermed = vsha512hq_u64(sum, vextq_u64(ef, gh, 1), vextq_u64(cd, ef, 1));
  gh = vsha512h2q_u64(intermed, cd, ab);
  cd = vaddq_u64(cd, intermed);

  /* Rounds 10 and 11 */
  initial_sum = vaddq_u64(s5, vld1q_u64(&K[10]));
  sum = vaddq_u64(vextq_u64(initial_sum, initial_sum, 1), ef);
  intermed = vsha512hq_u64(sum, vextq_u64(cd, ef, 1), vextq_u64(ab, cd, 1));
  ef = vsha512h2q_u64(intermed, ab, gh);
  ab = vaddq_u64(ab, intermed);

  /* Rounds 12 and 13 */
  initial_sum = vaddq_u64(s6, vld1q_u64(&K[12]));
  sum = vaddq_u64(vextq_u64(initial_sum, initial_sum, 1), cd);
  intermed = vsha512hq_u64(sum, vextq_u64(ab, cd, 1), vextq_u64(gh, ab, 1));
  cd = vsha512h2q_u64(intermed, gh, ef);
  gh = vaddq_u64(gh, intermed);

  /* Rounds 14 and 15 */
  initial_sum = vaddq_u64(s7, vld1q_u64(&K[14]));
  sum = vaddq_u64(vextq_u64(initial_sum, initial_sum, 1), ab);
  intermed = vsha512hq_u64(sum, vextq_u64(gh, ab, 1), vextq_u64(ef, gh, 1));
  ab = vsha512h2q_u64(intermed, ef, cd);
  ef = vaddq_u64(ef, intermed);

  /* Rounds 16-79: message schedule expansion + rounds */
  for (unsigned int t = 16; t < 80; t += 16) {
    /* Rounds t and t + 1 */
    s0 = vsha512su1q_u64(vsha512su0q_u64(s0, s1), s7, vextq_u64(s4, s5, 1));
    initial_sum = vaddq_u64(s0, vld1q_u64(&K[t]));
    sum = vaddq_u64(vextq_u64(initial_sum, initial_sum, 1), gh);
    intermed = vsha512hq_u64(sum, vextq_u64(ef, gh, 1), vextq_u64(cd, ef, 1));
    gh = vsha512h2q_u64(intermed, cd, ab);
    cd = vaddq_u64(cd, intermed);

    /* Rounds t + 2 and t + 3 */
    s1 = vsha512su1q_u64(vsha512su0q_u64(s1, s2), s0, vextq_u64(s5, s6, 1));
    initial_sum = vaddq_u64(s1, vld1q_u64(&K[t + 2]));
    sum = vaddq_u64(vextq_u64(initial_sum, initial_sum, 1), ef);
    intermed = vsha512hq_u64(sum, vextq_u64(cd, ef, 1), vextq_u64(ab, cd, 1));
    ef = vsha512h2q_u64(intermed, ab, gh);
    ab = vaddq_u64(ab, intermed);

    /* Rounds t + 4 and t + 5 */
    s2 = vsha512su1q_u64(vsha512su0q_u64(s2, s3), s1, vextq_u64(s6, s7, 1));
    initial_sum = vaddq_u64(s2, vld1q_u64(&K[t + 4]));
    sum = vaddq_u64(vextq_u64(initial_sum, initial_sum, 1), cd);
    intermed = vsha512hq_u64(sum, vextq_u64(ab, cd, 1), vextq_u64(gh, ab, 1));
    cd = vsha512h2q_u64(intermed, gh, ef);
    gh = vaddq_u64(gh, intermed);

    /* Rounds t + 6 and t + 7 */
    s3 = vsha512su1q_u64(vsha512su0q_u64(s3, s4), s2, vextq_u64(s7, s0, 1));
    initial_sum = vaddq_u64(s3, vld1q_u64(&K[t + 6]));
    sum = vaddq_u64(vextq_u64(initial_sum, initial_sum, 1), ab);
    intermed = vsha512hq_u64(sum, vextq_u64(gh, ab, 1), vextq_u64(ef, gh, 1));
    ab = vsha512h2q_u64(intermed, ef, cd);
    ef = vaddq_u64(ef, intermed);

    /* Rounds t + 8 and t + 9 */
    s4 = vsha512su1q_u64(vsha512su0q_u64(s4, s5), s3, vextq_u64(s0, s1, 1));
    initial_sum = vaddq_u64(s4, vld1q_u64(&K[t + 8]));
    sum = vaddq_u64(vextq_u64(initial_sum, initial_sum, 1), gh);
    intermed = vsha512hq_u64(sum, vextq_u64(ef, gh, 1), vextq_u64(cd, ef, 1));
    gh = vsha512h2q_u64(intermed, cd, ab);
    cd = vaddq_u64(cd, intermed);

    /* Rounds t + 10 and t + 11 */
    s5 = vsha512su1q_u64(vsha512su0q_u64(s5, s6), s4, vextq_u64(s1, s2, 1));
    initial_sum = vaddq_u64(s5, vld1q_u64(&K[t + 10]));
    sum = vaddq_u64(vextq_u64(initial_sum, initial_sum, 1), ef);
    intermed = vsha512hq_u64(sum, vextq_u64(cd, ef, 1), vextq_u64(ab, cd, 1));
    ef = vsha512h2q_u64(intermed, ab, gh);
    ab = vaddq_u64(ab, intermed);

    /* Rounds t + 12 and t + 13 */
    s6 = vsha512su1q_u64(vsha512su0q_u64(s6, s7), s5, vextq_u64(s2, s3, 1));
    initial_sum = vaddq_u64(s6, vld1q_u64(&K[t + 12]));
    sum = vaddq_u64(vextq_u64(initial_sum, initial_sum, 1), cd);
    intermed = vsha512hq_u64(sum, vextq_u64(ab, cd, 1), vextq_u64(gh, ab, 1));
    cd = vsha512h2q_u64(intermed, gh, ef);
    gh = vaddq_u64(gh, intermed);

    /* Rounds t + 14 and t + 15 */
    s7 = vsha512su1q_u64(vsha512su0q_u64(s7, s0), s6, vextq_u64(s3, s4, 1));
    initial_sum = vaddq_u64(s7, vld1q_u64(&K[t + 14]));
    sum = vaddq_u64(vextq_u64(initial_sum, initial_sum, 1), ab);
    intermed = vsha512hq_u64(sum, vextq_u64(gh, ab, 1), vextq_u64(ef, gh, 1));
    ab = vsha512h2q_u64(intermed, ef, cd);
    ef = vaddq_u64(ef, intermed);
  }

  /* Combine state */
  ab = vaddq_u64(ab, ab_orig);
  cd = vaddq_u64(cd, cd_orig);
  ef = vaddq_u64(ef, ef_orig);
  gh = vaddq_u64(gh, gh_orig);

  /* Save state */
  vst1q_u64(&h->u64[0], ab);
  vst1q_u64(&h->u64[2], cd);
  vst1q_u64(&h->u64[4], ef);
  vst1q_u64(&h->u64[6], gh);

#elif defined(FIO___HAS_X86_SHA512_INTRIN) && FIO___HAS_X86_SHA512_INTRIN
  /* x86 SHA-512 intrinsics implementation (Arrow Lake / Lunar Lake 2024+)
   * Uses _mm256_sha512rnds2_epi64, _mm256_sha512msg1_epi64,
   * _mm256_sha512msg2_epi64
   *
   * SHA-512 state: 8 x 64-bit words = 512 bits
   * Layout: state0 = {a, b, c, d}, state1 = {e, f, g, h}
   * Each __m256i holds 4 x 64-bit words
   *
   * _mm256_sha512rnds2_epi64 performs 2 rounds using lower 128 bits of k
   * _mm256_sha512msg1_epi64 computes sigma0 for message schedule
   * _mm256_sha512msg2_epi64 computes sigma1 for message schedule
   */
  static const uint64_t K[80] FIO_ALIGN(32) = {
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

  /* Byte-swap mask for big-endian conversion (64-bit word swap) */
  const __m256i shuf_mask = _mm256_set_epi8(8,
                                            9,
                                            10,
                                            11,
                                            12,
                                            13,
                                            14,
                                            15,
                                            0,
                                            1,
                                            2,
                                            3,
                                            4,
                                            5,
                                            6,
                                            7,
                                            8,
                                            9,
                                            10,
                                            11,
                                            12,
                                            13,
                                            14,
                                            15,
                                            0,
                                            1,
                                            2,
                                            3,
                                            4,
                                            5,
                                            6,
                                            7);

  /* Load state: state0 = {a, b, c, d}, state1 = {e, f, g, h} */
  __m256i state0 = _mm256_loadu_si256((const __m256i *)&h->u64[0]);
  __m256i state1 = _mm256_loadu_si256((const __m256i *)&h->u64[4]);

  /* Save original state for final addition */
  const __m256i state0_save = state0;
  const __m256i state1_save = state1;

  /* Load and byte-swap message (SHA-512 uses big-endian) */
  __m256i w0 =
      _mm256_shuffle_epi8(_mm256_loadu_si256((const __m256i *)(block + 0)),
                          shuf_mask);
  __m256i w1 =
      _mm256_shuffle_epi8(_mm256_loadu_si256((const __m256i *)(block + 32)),
                          shuf_mask);
  __m256i w2 =
      _mm256_shuffle_epi8(_mm256_loadu_si256((const __m256i *)(block + 64)),
                          shuf_mask);
  __m256i w3 =
      _mm256_shuffle_epi8(_mm256_loadu_si256((const __m256i *)(block + 96)),
                          shuf_mask);

  __m256i wk;

/* Macro for 2 rounds: uses lower 128 bits of wk for round constants */
#define FIO___SHA512_2RNDS(s0, s1, w, kptr)                                    \
  do {                                                                         \
    wk = _mm256_add_epi64(w, _mm256_loadu_si256((const __m256i *)(kptr)));     \
    s1 = _mm256_sha512rnds2_epi64(s1, s0, wk);                                 \
    wk = _mm256_permute4x64_epi64(wk, 0x4E);                                   \
    s0 = _mm256_sha512rnds2_epi64(s0, s1, wk);                                 \
  } while (0)

/* Macro for message schedule: W[i] = sigma1(W[i-2]) + W[i-7] + sigma0(W[i-15])
 * + W[i-16] */
#define FIO___SHA512_MSG_SCHED(w, w1, w2, w3)                                  \
  do {                                                                         \
    __m256i t0 = _mm256_sha512msg1_epi64(w, w1);                               \
    __m256i t1 = _mm256_alignr_epi8(w3, w2, 8);                                \
    t0 = _mm256_add_epi64(t0, t1);                                             \
    w = _mm256_sha512msg2_epi64(t0, w3);                                       \
  } while (0)

  /* Rounds 0-15: use initial message words directly */
  FIO___SHA512_2RNDS(state0, state1, w0, &K[0]);
  FIO___SHA512_2RNDS(state0, state1, w1, &K[4]);
  FIO___SHA512_2RNDS(state0, state1, w2, &K[8]);
  FIO___SHA512_2RNDS(state0, state1, w3, &K[12]);

  /* Rounds 16-79: message schedule expansion + rounds */
  for (unsigned int t = 16; t < 80; t += 16) {
    FIO___SHA512_MSG_SCHED(w0, w1, w2, w3);
    FIO___SHA512_2RNDS(state0, state1, w0, &K[t]);

    FIO___SHA512_MSG_SCHED(w1, w2, w3, w0);
    FIO___SHA512_2RNDS(state0, state1, w1, &K[t + 4]);

    FIO___SHA512_MSG_SCHED(w2, w3, w0, w1);
    FIO___SHA512_2RNDS(state0, state1, w2, &K[t + 8]);

    FIO___SHA512_MSG_SCHED(w3, w0, w1, w2);
    FIO___SHA512_2RNDS(state0, state1, w3, &K[t + 12]);
  }

#undef FIO___SHA512_2RNDS
#undef FIO___SHA512_MSG_SCHED

  /* Combine state */
  state0 = _mm256_add_epi64(state0, state0_save);
  state1 = _mm256_add_epi64(state1, state1_save);

  /* Save state */
  _mm256_storeu_si256((__m256i *)&h->u64[0], state0);
  _mm256_storeu_si256((__m256i *)&h->u64[4], state1);

#else /* Portable implementation */
  /* SHA-512 round constants */
  static const uint64_t K[80] FIO_ALIGN(64) = {
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

  /* Load state into registers */
  uint64_t a = h->u64[0], b = h->u64[1], c = h->u64[2], d = h->u64[3];
  uint64_t e = h->u64[4], f = h->u64[5], g = h->u64[6], hv = h->u64[7];

  /* Message schedule — use fio_u1024 for aligned storage */
  fio_u1024 wv;
  fio_memcpy128(wv.u8, block);
  /* Byte-swap to big-endian (no-op on big-endian systems) */
  for (size_t i = 0; i < 16; ++i)
    wv.u64[i] = fio_lton64(wv.u64[i]);
  uint64_t *w = wv.u64;

/* SHA-512 Sigma functions - using optimized helpers */
#define FIO___S512_S0(x)        fio_xor_rrot3_64(x, 28, 34, 39)
#define FIO___S512_S1(x)        fio_xor_rrot3_64(x, 14, 18, 41)
#define FIO___S512_s0(x)        fio_xor_rrot2_shr_64(x, 1, 8, 7)
#define FIO___S512_s1(x)        fio_xor_rrot2_shr_64(x, 19, 61, 6)

/* Optimized Ch and Maj - fewer operations */
#define FIO___S512_CH(e, f, g)  ((g) ^ ((e) & ((f) ^ (g))))
#define FIO___S512_MAJ(a, b, c) (((a) & (b)) | ((c) & ((a) | (b))))

/* SHA-512 round - rotates variables instead of copying */
#define FIO___SHA512_RND(a, b, c, d, e, f, g, h, k, w)                         \
  do {                                                                         \
    uint64_t t1 = (h) + FIO___S512_S1(e) + FIO___S512_CH(e, f, g) + (k) + (w); \
    uint64_t t2 = FIO___S512_S0(a) + FIO___S512_MAJ(a, b, c);                  \
    (d) += t1;                                                                 \
    (h) = t1 + t2;                                                             \
  } while (0)

/* Message schedule expansion */
#define FIO___SHA512_SCHED(i)                                                  \
  (w[(i)&15] += FIO___S512_s0(w[((i) + 1) & 15]) + w[((i) + 9) & 15] +         \
                FIO___S512_s1(w[((i) + 14) & 15]))

  /* Rounds 0-15 - use message words directly */
  FIO___SHA512_RND(a, b, c, d, e, f, g, hv, K[0], w[0]);
  FIO___SHA512_RND(hv, a, b, c, d, e, f, g, K[1], w[1]);
  FIO___SHA512_RND(g, hv, a, b, c, d, e, f, K[2], w[2]);
  FIO___SHA512_RND(f, g, hv, a, b, c, d, e, K[3], w[3]);
  FIO___SHA512_RND(e, f, g, hv, a, b, c, d, K[4], w[4]);
  FIO___SHA512_RND(d, e, f, g, hv, a, b, c, K[5], w[5]);
  FIO___SHA512_RND(c, d, e, f, g, hv, a, b, K[6], w[6]);
  FIO___SHA512_RND(b, c, d, e, f, g, hv, a, K[7], w[7]);
  FIO___SHA512_RND(a, b, c, d, e, f, g, hv, K[8], w[8]);
  FIO___SHA512_RND(hv, a, b, c, d, e, f, g, K[9], w[9]);
  FIO___SHA512_RND(g, hv, a, b, c, d, e, f, K[10], w[10]);
  FIO___SHA512_RND(f, g, hv, a, b, c, d, e, K[11], w[11]);
  FIO___SHA512_RND(e, f, g, hv, a, b, c, d, K[12], w[12]);
  FIO___SHA512_RND(d, e, f, g, hv, a, b, c, K[13], w[13]);
  FIO___SHA512_RND(c, d, e, f, g, hv, a, b, K[14], w[14]);
  FIO___SHA512_RND(b, c, d, e, f, g, hv, a, K[15], w[15]);

  /* Rounds 16-31 */
  FIO___SHA512_RND(a, b, c, d, e, f, g, hv, K[16], FIO___SHA512_SCHED(0));
  FIO___SHA512_RND(hv, a, b, c, d, e, f, g, K[17], FIO___SHA512_SCHED(1));
  FIO___SHA512_RND(g, hv, a, b, c, d, e, f, K[18], FIO___SHA512_SCHED(2));
  FIO___SHA512_RND(f, g, hv, a, b, c, d, e, K[19], FIO___SHA512_SCHED(3));
  FIO___SHA512_RND(e, f, g, hv, a, b, c, d, K[20], FIO___SHA512_SCHED(4));
  FIO___SHA512_RND(d, e, f, g, hv, a, b, c, K[21], FIO___SHA512_SCHED(5));
  FIO___SHA512_RND(c, d, e, f, g, hv, a, b, K[22], FIO___SHA512_SCHED(6));
  FIO___SHA512_RND(b, c, d, e, f, g, hv, a, K[23], FIO___SHA512_SCHED(7));
  FIO___SHA512_RND(a, b, c, d, e, f, g, hv, K[24], FIO___SHA512_SCHED(8));
  FIO___SHA512_RND(hv, a, b, c, d, e, f, g, K[25], FIO___SHA512_SCHED(9));
  FIO___SHA512_RND(g, hv, a, b, c, d, e, f, K[26], FIO___SHA512_SCHED(10));
  FIO___SHA512_RND(f, g, hv, a, b, c, d, e, K[27], FIO___SHA512_SCHED(11));
  FIO___SHA512_RND(e, f, g, hv, a, b, c, d, K[28], FIO___SHA512_SCHED(12));
  FIO___SHA512_RND(d, e, f, g, hv, a, b, c, K[29], FIO___SHA512_SCHED(13));
  FIO___SHA512_RND(c, d, e, f, g, hv, a, b, K[30], FIO___SHA512_SCHED(14));
  FIO___SHA512_RND(b, c, d, e, f, g, hv, a, K[31], FIO___SHA512_SCHED(15));

  /* Rounds 32-47 */
  FIO___SHA512_RND(a, b, c, d, e, f, g, hv, K[32], FIO___SHA512_SCHED(0));
  FIO___SHA512_RND(hv, a, b, c, d, e, f, g, K[33], FIO___SHA512_SCHED(1));
  FIO___SHA512_RND(g, hv, a, b, c, d, e, f, K[34], FIO___SHA512_SCHED(2));
  FIO___SHA512_RND(f, g, hv, a, b, c, d, e, K[35], FIO___SHA512_SCHED(3));
  FIO___SHA512_RND(e, f, g, hv, a, b, c, d, K[36], FIO___SHA512_SCHED(4));
  FIO___SHA512_RND(d, e, f, g, hv, a, b, c, K[37], FIO___SHA512_SCHED(5));
  FIO___SHA512_RND(c, d, e, f, g, hv, a, b, K[38], FIO___SHA512_SCHED(6));
  FIO___SHA512_RND(b, c, d, e, f, g, hv, a, K[39], FIO___SHA512_SCHED(7));
  FIO___SHA512_RND(a, b, c, d, e, f, g, hv, K[40], FIO___SHA512_SCHED(8));
  FIO___SHA512_RND(hv, a, b, c, d, e, f, g, K[41], FIO___SHA512_SCHED(9));
  FIO___SHA512_RND(g, hv, a, b, c, d, e, f, K[42], FIO___SHA512_SCHED(10));
  FIO___SHA512_RND(f, g, hv, a, b, c, d, e, K[43], FIO___SHA512_SCHED(11));
  FIO___SHA512_RND(e, f, g, hv, a, b, c, d, K[44], FIO___SHA512_SCHED(12));
  FIO___SHA512_RND(d, e, f, g, hv, a, b, c, K[45], FIO___SHA512_SCHED(13));
  FIO___SHA512_RND(c, d, e, f, g, hv, a, b, K[46], FIO___SHA512_SCHED(14));
  FIO___SHA512_RND(b, c, d, e, f, g, hv, a, K[47], FIO___SHA512_SCHED(15));

  /* Rounds 48-63 */
  FIO___SHA512_RND(a, b, c, d, e, f, g, hv, K[48], FIO___SHA512_SCHED(0));
  FIO___SHA512_RND(hv, a, b, c, d, e, f, g, K[49], FIO___SHA512_SCHED(1));
  FIO___SHA512_RND(g, hv, a, b, c, d, e, f, K[50], FIO___SHA512_SCHED(2));
  FIO___SHA512_RND(f, g, hv, a, b, c, d, e, K[51], FIO___SHA512_SCHED(3));
  FIO___SHA512_RND(e, f, g, hv, a, b, c, d, K[52], FIO___SHA512_SCHED(4));
  FIO___SHA512_RND(d, e, f, g, hv, a, b, c, K[53], FIO___SHA512_SCHED(5));
  FIO___SHA512_RND(c, d, e, f, g, hv, a, b, K[54], FIO___SHA512_SCHED(6));
  FIO___SHA512_RND(b, c, d, e, f, g, hv, a, K[55], FIO___SHA512_SCHED(7));
  FIO___SHA512_RND(a, b, c, d, e, f, g, hv, K[56], FIO___SHA512_SCHED(8));
  FIO___SHA512_RND(hv, a, b, c, d, e, f, g, K[57], FIO___SHA512_SCHED(9));
  FIO___SHA512_RND(g, hv, a, b, c, d, e, f, K[58], FIO___SHA512_SCHED(10));
  FIO___SHA512_RND(f, g, hv, a, b, c, d, e, K[59], FIO___SHA512_SCHED(11));
  FIO___SHA512_RND(e, f, g, hv, a, b, c, d, K[60], FIO___SHA512_SCHED(12));
  FIO___SHA512_RND(d, e, f, g, hv, a, b, c, K[61], FIO___SHA512_SCHED(13));
  FIO___SHA512_RND(c, d, e, f, g, hv, a, b, K[62], FIO___SHA512_SCHED(14));
  FIO___SHA512_RND(b, c, d, e, f, g, hv, a, K[63], FIO___SHA512_SCHED(15));

  /* Rounds 64-79 */
  FIO___SHA512_RND(a, b, c, d, e, f, g, hv, K[64], FIO___SHA512_SCHED(0));
  FIO___SHA512_RND(hv, a, b, c, d, e, f, g, K[65], FIO___SHA512_SCHED(1));
  FIO___SHA512_RND(g, hv, a, b, c, d, e, f, K[66], FIO___SHA512_SCHED(2));
  FIO___SHA512_RND(f, g, hv, a, b, c, d, e, K[67], FIO___SHA512_SCHED(3));
  FIO___SHA512_RND(e, f, g, hv, a, b, c, d, K[68], FIO___SHA512_SCHED(4));
  FIO___SHA512_RND(d, e, f, g, hv, a, b, c, K[69], FIO___SHA512_SCHED(5));
  FIO___SHA512_RND(c, d, e, f, g, hv, a, b, K[70], FIO___SHA512_SCHED(6));
  FIO___SHA512_RND(b, c, d, e, f, g, hv, a, K[71], FIO___SHA512_SCHED(7));
  FIO___SHA512_RND(a, b, c, d, e, f, g, hv, K[72], FIO___SHA512_SCHED(8));
  FIO___SHA512_RND(hv, a, b, c, d, e, f, g, K[73], FIO___SHA512_SCHED(9));
  FIO___SHA512_RND(g, hv, a, b, c, d, e, f, K[74], FIO___SHA512_SCHED(10));
  FIO___SHA512_RND(f, g, hv, a, b, c, d, e, K[75], FIO___SHA512_SCHED(11));
  FIO___SHA512_RND(e, f, g, hv, a, b, c, d, K[76], FIO___SHA512_SCHED(12));
  FIO___SHA512_RND(d, e, f, g, hv, a, b, c, K[77], FIO___SHA512_SCHED(13));
  FIO___SHA512_RND(c, d, e, f, g, hv, a, b, K[78], FIO___SHA512_SCHED(14));
  FIO___SHA512_RND(b, c, d, e, f, g, hv, a, K[79], FIO___SHA512_SCHED(15));

#undef FIO___S512_S0
#undef FIO___S512_S1
#undef FIO___S512_s0
#undef FIO___S512_s1
#undef FIO___S512_CH
#undef FIO___S512_MAJ
#undef FIO___SHA512_RND
#undef FIO___SHA512_SCHED

  /* Add to hash state */
  h->u64[0] += a;
  h->u64[1] += b;
  h->u64[2] += c;
  h->u64[3] += d;
  h->u64[4] += e;
  h->u64[5] += f;
  h->u64[6] += g;
  h->u64[7] += hv;

#endif /* FIO___HAS_ARM_SHA512_INTRIN */
}

/** Feed data into the hash */
SFUNC void fio_sha512_consume(fio_sha512_s *restrict h,
                              const void *restrict data,
                              uint64_t len) {
  if (!len)
    return;
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
