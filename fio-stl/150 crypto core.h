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
Module Implementation - possibly externed functions.
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/* *****************************************************************************
Module Cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_CRYPTO_CORE
#endif /* FIO_CRYPTO_CORE */
