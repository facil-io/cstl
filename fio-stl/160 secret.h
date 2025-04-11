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

/** Gets the SHA512 of a (possibly shared) secret. */
SFUNC fio_u512 fio_secret(void) { return fio___secret; }

/** Sets a (possibly shared) secret and stores its SHA512 hash. */
SFUNC void fio_secret_set(char *str, size_t len, bool is_random) {
  if (!str)
    return;
  fio___secret_is_random = is_random;
  fio___secret = fio_sha512(str, len);
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
