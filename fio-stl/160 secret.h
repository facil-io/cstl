/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_CRYPTO_CORE        /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                  A Template for New Types / Modules




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_SECRET) && !defined(H___FIO_SECRET___H)
#define H___FIO_SECRET___H

/** Returns true if the secret was randomly generated. */
SFUNC bool fio_secret_is_random(void);

/** Gets the SHA512 of a (possibly shared) secret. */
SFUNC fio_u512 fio_secret(void);

/** Sets a (possibly shared) secret and stores its SHA512 hash. */
SFUNC void fio_secret_set(char *str, size_t len, bool is_random);

/* *****************************************************************************
Module Implementation - possibly externed functions.
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

static fio_u512 fio___secret;
static bool fio___secret_is_random;
static uint64_t fio___secret_masker;

/** Returns true if the secret was randomly generated. */
SFUNC bool fio_secret_is_random(void) { return fio___secret_is_random; }

/** Gets the SHA512 of a (possibly shared) secret. */
SFUNC fio_u512 fio_secret(void) {
  fio_u512 r;
  fio_u512_cxor64(&r, &fio___secret, fio___secret_masker);
  return r;
}

/** Sets a (possibly shared) secret and stores its SHA512 hash. */
SFUNC void fio_secret_set(char *str, size_t len, bool is_random) {
  if (!str || !len)
    return;
  fio_u512 zero = {0};
  size_t i = 0;
  FIO_STR_INFO_TMP_VAR(from_hex, 4096);
  if (len > 8191)
    goto done;
  /* convert any Hex data to bytes */
  for (i = 0; i + 1 < len; i += 2) {
    /* skip white space */
    if (str[i] == ' ' || str[i] == '\t' || str[i] == '\n' || str[i] == '\r') {
      --i;
      continue;
    }
    const size_t hi = fio_c2i(str[i]);
    const size_t lo = fio_c2i(str[i + 1]);
    if ((unsigned)(hi > 15) | (lo > 15))
      goto done;
    from_hex.buf[from_hex.len++] = (hi << 4) | lo;
  }
  from_hex.buf[from_hex.len] = 0;
  while (i < len &&
         (str[i] == ' ' || str[i] == '\t' || str[i] == '\n' || str[i] == '\r'))
    ++i;
  if (i == len) { /* test if the secret was all Hex encoded */
    FIO_LOG_DDEBUG2("(secret) Decoded HEX encoded secret (%zu bytes).",
                    from_hex.len);
    str = from_hex.buf;
    len = from_hex.len;
  }

done:
  fio___secret_is_random = is_random;
  fio___secret = fio_sha512(str, len);
  if (fio_u512_is_eq(&zero, &fio___secret)) {
    fio___secret.u64[0] = len;
    fio___secret = fio_sha512(fio___secret.u8, sizeof(fio___secret));
  }
  while (!(fio___secret_masker = fio_rand64()))
    ;
  fio_u512_cxor64(&fio___secret, &fio___secret, fio___secret_masker);
}

FIO_CONSTRUCTOR(fio___secret_constructor) {
  char *str = NULL;
  size_t len = 0;
  uint64_t fallback_secret = 0;
  bool is_random = 0;
  if ((str = getenv("SECRET"))) {
    const char *secret_length = getenv("SECRET_LENGTH");
    len = secret_length ? fio_atol((char **)&secret_length) : 0;
    if (!len)
      len = strlen(str);
  } else {
    fallback_secret = fio_rand64();
    str = (char *)&fallback_secret;
    len = sizeof(fallback_secret);
    is_random = 1;
  }
  fio_secret_set(str, len, is_random);
}
/* *****************************************************************************
Module Cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_SECRET
#endif /* FIO_CRYPTO_CORE */
