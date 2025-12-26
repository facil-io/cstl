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

Note: HKDF requires SHA-2 HMAC functions (fio_sha256_hmac, fio_sha512_hmac).
      Either define FIO_SHA2 before FIO_HKDF, or use FIO_CRYPTO to include all
      crypto modules.
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
 * @param ikm Input keying material
 * @param ikm_len IKM length in bytes
 * @param use_sha384 If non-zero, use SHA-384; otherwise SHA-256
 */
SFUNC void fio_hkdf_extract(void *restrict prk,
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
 * @param prk Pseudorandom key from Extract (32 or 48 bytes)
 * @param prk_len PRK length (32 for SHA-256, 48 for SHA-384)
 * @param info Optional context/application info
 * @param info_len Info length in bytes
 * @param use_sha384 If non-zero, use SHA-384; otherwise SHA-256
 */
SFUNC void fio_hkdf_expand(void *restrict okm,
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
 * @param ikm Input keying material
 * @param ikm_len IKM length in bytes
 * @param info Optional context/application info
 * @param info_len Info length in bytes
 * @param use_sha384 If non-zero, use SHA-384; otherwise SHA-256
 */
SFUNC void fio_hkdf(void *restrict okm,
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

SFUNC void fio_hkdf_extract(void *restrict prk,
                            const void *restrict salt,
                            size_t salt_len,
                            const void *restrict ikm,
                            size_t ikm_len,
                            int use_sha384) {
  if (!prk || !ikm)
    return;

  if (use_sha384) {
    /* SHA-384: use SHA-512 HMAC, truncate to 48 bytes */
    /* If salt is NULL or empty, use hash_len zeros */
    uint8_t zero_salt[48] = {0};
    const void *actual_salt = salt;
    size_t actual_salt_len = salt_len;
    if (!salt || salt_len == 0) {
      actual_salt = zero_salt;
      actual_salt_len = 48;
    }
    /* PRK = HMAC-SHA384(salt, IKM) - using SHA-512 HMAC truncated */
    fio_u512 hmac_result =
        fio_sha512_hmac(actual_salt, actual_salt_len, ikm, ikm_len);
    /* Copy first 48 bytes (SHA-384 output) */
    FIO_MEMCPY(prk, hmac_result.u8, 48);
  } else {
    /* SHA-256 */
    uint8_t zero_salt[32] = {0};
    const void *actual_salt = salt;
    size_t actual_salt_len = salt_len;
    if (!salt || salt_len == 0) {
      actual_salt = zero_salt;
      actual_salt_len = 32;
    }
    /* PRK = HMAC-SHA256(salt, IKM) */
    fio_u256 hmac_result =
        fio_sha256_hmac(actual_salt, actual_salt_len, ikm, ikm_len);
    fio_memcpy32(prk, hmac_result.u8);
  }
}

/* *****************************************************************************
HKDF-Expand Implementation
***************************************************************************** */

SFUNC void fio_hkdf_expand(void *restrict okm,
                           size_t okm_len,
                           const void *restrict prk,
                           size_t prk_len,
                           const void *restrict info,
                           size_t info_len,
                           int use_sha384) {
  if (!okm || !prk || okm_len == 0)
    return;

  const size_t hash_len = use_sha384 ? 48 : 32;
  const size_t max_okm_len = 255 * hash_len;

  /* Clamp output length to maximum allowed */
  if (okm_len > max_okm_len)
    okm_len = max_okm_len;

  uint8_t *out = (uint8_t *)okm;
  uint8_t t_prev[64] = {0}; /* T(i-1), max 64 bytes for SHA-512 */
  size_t t_prev_len = 0;
  uint8_t counter = 1;
  size_t remaining = okm_len;

  /* N = ceil(L/HashLen) iterations */
  while (remaining > 0) {
    /* T(i) = HMAC-Hash(PRK, T(i-1) || info || counter) */
    /* Build input: T(i-1) || info || counter */
    size_t input_len = t_prev_len + info_len + 1;
    uint8_t *input = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, input_len, 0);
    if (!input)
      return;

    size_t offset = 0;
    if (t_prev_len > 0) {
      FIO_MEMCPY(input, t_prev, t_prev_len);
      offset = t_prev_len;
    }
    if (info && info_len > 0) {
      FIO_MEMCPY(input + offset, info, info_len);
      offset += info_len;
    }
    input[offset] = counter;

    if (use_sha384) {
      fio_u512 hmac_result = fio_sha512_hmac(prk, prk_len, input, input_len);
      FIO_MEMCPY(t_prev, hmac_result.u8, 48);
      t_prev_len = 48;
    } else {
      fio_u256 hmac_result = fio_sha256_hmac(prk, prk_len, input, input_len);
      fio_memcpy32(t_prev, hmac_result.u8);
      t_prev_len = 32;
    }

    FIO_MEM_FREE(input, input_len);

    /* Copy to output */
    size_t to_copy = (remaining < hash_len) ? remaining : hash_len;
    FIO_MEMCPY(out, t_prev, to_copy);
    out += to_copy;
    remaining -= to_copy;
    ++counter;
  }
}

/* *****************************************************************************
Combined HKDF Implementation
***************************************************************************** */

SFUNC void fio_hkdf(void *restrict okm,
                    size_t okm_len,
                    const void *restrict salt,
                    size_t salt_len,
                    const void *restrict ikm,
                    size_t ikm_len,
                    const void *restrict info,
                    size_t info_len,
                    int use_sha384) {
  if (!okm || !ikm || okm_len == 0)
    return;

  const size_t prk_len = use_sha384 ? 48 : 32;
  uint8_t prk[48]; /* Max size for SHA-384 */

  /* Step 1: Extract */
  fio_hkdf_extract(prk, salt, salt_len, ikm, ikm_len, use_sha384);

  /* Step 2: Expand */
  fio_hkdf_expand(okm, okm_len, prk, prk_len, info, info_len, use_sha384);
}

/* *****************************************************************************
Module Cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_HKDF */
#undef FIO_HKDF
