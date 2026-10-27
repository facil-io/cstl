/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_HKDF               /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                                    HKDF
                    HMAC-based Key Derivation Function (RFC 5869)




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_HKDF) && !defined(H___FIO_HKDF___H)
#define H___FIO_HKDF___H

/* *****************************************************************************
HKDF API

Note: HKDF requires the SHA-2 module (SHA-256 / SHA-384 and their HMAC).
      Either define FIO_SHA2 before FIO_HKDF, or use FIO_CRYPTO to include
      all crypto modules.
***************************************************************************** */

/** SHA-256 hash length (32 bytes). */
#define FIO_HKDF_SHA256_HASH_LEN 32
/** SHA-384 hash length (48 bytes). */
#define FIO_HKDF_SHA384_HASH_LEN 48

/**
 * HKDF-Extract: PRK = HMAC-Hash(salt, IKM)
 *
 * Extracts a pseudorandom key (PRK) from input keying material (IKM).
 *
 * @param prk Output buffer (32 bytes for SHA-256, 48 for SHA-384)
 * @param salt Optional salt (if NULL, uses zeros of hash length)
 * @param salt_len Salt length in bytes
 * @param ikm Input keying material (may be NULL only if `ikm_len` is 0)
 * @param ikm_len IKM length in bytes
 * @param use_sha384 If non-zero, use SHA-384; otherwise SHA-256
 * @return 0 on success; -1 (PRK untouched) if `prk` is NULL or `ikm` is NULL
 *         with a non-zero `ikm_len`.
 */
SFUNC int fio_hkdf_extract(void *restrict prk,
                           const void *restrict salt,
                           size_t salt_len,
                           const void *restrict ikm,
                           size_t ikm_len,
                           int use_sha384);

/**
 * HKDF-Expand: OKM = HKDF-Expand(PRK, info, L)
 *
 * Expands a pseudorandom key (PRK) into output keying material (OKM).
 *
 * @param okm Output keying material buffer
 * @param okm_len Desired output length (max 255 * hash_len)
 * @param prk Pseudorandom key from Extract (at least hash_len bytes)
 * @param prk_len PRK length (32 for SHA-256, 48 for SHA-384)
 * @param info Optional context/application info (NULL is empty info)
 * @param info_len Info length in bytes
 * @param use_sha384 If non-zero, use SHA-384; otherwise SHA-256
 * @return 0 on success; -1 (OKM untouched) if `okm_len > 255 * hash_len`,
 *         `prk_len < hash_len`, or a required pointer is NULL.
 */
SFUNC int fio_hkdf_expand(void *restrict okm,
                          size_t okm_len,
                          const void *restrict prk,
                          size_t prk_len,
                          const void *restrict info,
                          size_t info_len,
                          int use_sha384);

/**
 * Combined HKDF (Extract + Expand) - RFC 5869 Section 2.
 *
 * Derives keying material from input keying material using HKDF.
 *
 * @param okm Output keying material buffer
 * @param okm_len Desired output length (max 255 * hash_len)
 * @param salt Optional salt (if NULL, uses zeros of hash length)
 * @param salt_len Salt length in bytes
 * @param ikm Input keying material (may be NULL only if `ikm_len` is 0)
 * @param ikm_len IKM length in bytes
 * @param info Optional context/application info (NULL is empty info)
 * @param info_len Info length in bytes
 * @param use_sha384 If non-zero, use SHA-384; otherwise SHA-256
 * @return 0 on success; -1 (OKM untouched) on invalid arguments.
 */
SFUNC int fio_hkdf(void *restrict okm,
                   size_t okm_len,
                   const void *restrict salt,
                   size_t salt_len,
                   const void *restrict ikm,
                   size_t ikm_len,
                   const void *restrict info,
                   size_t info_len,
                   int use_sha384);

/* *****************************************************************************
Implementation - possibly externed functions.
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/* *****************************************************************************
HKDF-Extract Implementation
***************************************************************************** */

SFUNC int fio_hkdf_extract(void *restrict prk,
                           const void *restrict salt,
                           size_t salt_len,
                           const void *restrict ikm,
                           size_t ikm_len,
                           int use_sha384) {
  if (!prk || (!ikm && ikm_len))
    return -1;
  if (!ikm)
    ikm = (const void *)"";
  /* NULL / empty salt: HMAC zero-pads keys, so an empty key equals the
   * RFC 5869 default (HashLen zero bytes). */
  if (!salt) {
    salt = (const void *)"";
    salt_len = 0;
  }
  if (use_sha384) {
    fio_u512 r = fio_sha384_hmac(salt, salt_len, ikm, ikm_len);
    FIO_MEMCPY(prk, r.u8, 48);
    fio_secure_zero(&r, sizeof(r));
  } else {
    fio_u256 r = fio_sha256_hmac(salt, salt_len, ikm, ikm_len);
    fio_memcpy32(prk, r.u8);
    fio_secure_zero(&r, sizeof(r));
  }
  return 0;
}

/* *****************************************************************************
HKDF-Expand Implementation

The HMAC inner / outer key blocks are hashed once and the keyed states are
reused for every T(i): no allocation, and 2 fewer compressions per block.
***************************************************************************** */

/* T(i) = HMAC-SHA256(PRK, T(i-1) || info || i). Arguments pre-validated. */
FIO_SFUNC void fio___hkdf_expand256(uint8_t *out,
                                    size_t len,
                                    const void *prk,
                                    size_t prk_len,
                                    const void *info,
                                    size_t info_len) {
  fio_u512 k = {0};
  fio_sha256_s ipad, opad, h;
  fio_u256 t;
  uint8_t counter = 1;
  if (prk_len > 64)
    k.u256[0] = fio_sha256(prk, prk_len);
  else
    FIO_MEMCPY(k.u8, prk, prk_len);
  for (size_t i = 0; i < 8; ++i)
    k.u64[i] ^= (uint64_t)0x3636363636363636ULL;
  ipad = fio_sha256_init();
  fio_sha256_consume(&ipad, k.u8, 64);
  for (size_t i = 0; i < 8; ++i)
    k.u64[i] ^=
        ((uint64_t)0x3636363636363636ULL ^ (uint64_t)0x5C5C5C5C5C5C5C5CULL);
  opad = fio_sha256_init();
  fio_sha256_consume(&opad, k.u8, 64);
  for (;;) {
    h = ipad;
    if (counter > 1)
      fio_sha256_consume(&h, t.u8, 32);
    fio_sha256_consume(&h, info, info_len);
    fio_sha256_consume(&h, &counter, 1);
    t = fio_sha256_finalize(&h);
    h = opad;
    fio_sha256_consume(&h, t.u8, 32);
    t = fio_sha256_finalize(&h);
    if (len <= 32) {
      FIO_MEMCPY(out, t.u8, len);
      break;
    }
    fio_memcpy32(out, t.u8);
    out += 32;
    len -= 32;
    ++counter;
  }
  fio_secure_zero(&k, sizeof(k));
  fio_secure_zero(&ipad, sizeof(ipad));
  fio_secure_zero(&opad, sizeof(opad));
  fio_secure_zero(&h, sizeof(h));
  fio_secure_zero(&t, sizeof(t));
}

/* T(i) = HMAC-SHA384(PRK, T(i-1) || info || i). Arguments pre-validated. */
FIO_SFUNC void fio___hkdf_expand384(uint8_t *out,
                                    size_t len,
                                    const void *prk,
                                    size_t prk_len,
                                    const void *info,
                                    size_t info_len) {
  fio_u1024 k = {0};
  fio_sha512_s ipad, opad, h;
  fio_u512 t;
  uint8_t counter = 1;
  if (prk_len > 128) {
    k.u512[0] = fio_sha384(prk, prk_len);
    FIO_MEMSET(k.u8 + 48, 0, 16); /* only 48 bytes of the result are hash */
  } else
    FIO_MEMCPY(k.u8, prk, prk_len);
  for (size_t i = 0; i < 16; ++i)
    k.u64[i] ^= (uint64_t)0x3636363636363636ULL;
  ipad = fio_sha384_init();
  fio_sha384_consume(&ipad, k.u8, 128);
  for (size_t i = 0; i < 16; ++i)
    k.u64[i] ^=
        ((uint64_t)0x3636363636363636ULL ^ (uint64_t)0x5C5C5C5C5C5C5C5CULL);
  opad = fio_sha384_init();
  fio_sha384_consume(&opad, k.u8, 128);
  for (;;) {
    h = ipad;
    if (counter > 1)
      fio_sha384_consume(&h, t.u8, 48);
    fio_sha384_consume(&h, info, info_len);
    fio_sha384_consume(&h, &counter, 1);
    t = fio_sha384_finalize(&h);
    h = opad;
    fio_sha384_consume(&h, t.u8, 48);
    t = fio_sha384_finalize(&h);
    if (len <= 48) {
      FIO_MEMCPY(out, t.u8, len);
      break;
    }
    FIO_MEMCPY(out, t.u8, 48);
    out += 48;
    len -= 48;
    ++counter;
  }
  fio_secure_zero(&k, sizeof(k));
  fio_secure_zero(&ipad, sizeof(ipad));
  fio_secure_zero(&opad, sizeof(opad));
  fio_secure_zero(&h, sizeof(h));
  fio_secure_zero(&t, sizeof(t));
}

SFUNC int fio_hkdf_expand(void *restrict okm,
                          size_t okm_len,
                          const void *restrict prk,
                          size_t prk_len,
                          const void *restrict info,
                          size_t info_len,
                          int use_sha384) {
  const size_t hash_len = use_sha384 ? 48 : 32;
  /* validate everything first - on error OKM is never written */
  if (!prk || prk_len < hash_len || okm_len > 255 * hash_len ||
      (!okm && okm_len))
    return -1;
  if (!okm_len)
    return 0;
  if (!info)
    info_len = 0; /* NULL info is empty info (as with NULL salt) */
  if (use_sha384)
    fio___hkdf_expand384((uint8_t *)okm, okm_len, prk, prk_len, info, info_len);
  else
    fio___hkdf_expand256((uint8_t *)okm, okm_len, prk, prk_len, info, info_len);
  return 0;
}

/* *****************************************************************************
Combined HKDF Implementation
***************************************************************************** */

SFUNC int fio_hkdf(void *restrict okm,
                   size_t okm_len,
                   const void *restrict salt,
                   size_t salt_len,
                   const void *restrict ikm,
                   size_t ikm_len,
                   const void *restrict info,
                   size_t info_len,
                   int use_sha384) {
  const size_t prk_len = use_sha384 ? 48 : 32;
  uint8_t prk[48]; /* Max size for SHA-384 */
  int r;
  /* validate output first - on error OKM is never written */
  if (okm_len > 255 * prk_len || (!okm && okm_len))
    return -1;
  r = fio_hkdf_extract(prk, salt, salt_len, ikm, ikm_len, use_sha384);
  if (!r)
    r = fio_hkdf_expand(okm, okm_len, prk, prk_len, info, info_len, use_sha384);
  fio_secure_zero(prk, sizeof(prk));
  return r;
}

/* *****************************************************************************
Module Cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_HKDF */
#undef FIO_HKDF
