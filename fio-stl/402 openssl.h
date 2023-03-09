/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_SERVER             /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                  OpenSSL Implementation for IO Functions




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_SERVER) && HAVE_OPENSSL && !defined(FIO___RECURSIVE_INCLUDE)

/* *****************************************************************************
OpenSSL Helper Settings
***************************************************************************** */

typedef struct {
  /* module's type(s) if any */
  SSL_CTX *ctx;
} fio_tls_openssl_s;

/* *****************************************************************************
OpenSSL Helpers Implementation - inlined static functions
***************************************************************************** */
/*
REMEMBER:
========

All memory allocations should use:
* FIO_MEM_REALLOC_(ptr, old_size, new_size, copy_len)
* FIO_MEM_FREE_(ptr, size)

*/

/* *****************************************************************************
OpenSSL Helpers Implementation - possibly externed functions.
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

#include <openssl/err.h>
#include <openssl/ssl.h>

/*
REMEMBER:
========

All memory allocations should use:
* FIO_MEM_REALLOC_(ptr, old_size, new_size, copy_len)
* FIO_MEM_FREE_(ptr, size)

*/
#if 0
SSL_CTX *create_context(bool isServer) {
  const SSL_METHOD *method;
  SSL_CTX *ctx;

  if (isServer)
    method = TLS_server_method();
  else
    method = TLS_client_method();

  ctx = SSL_CTX_new(method);
  if (ctx == NULL) {
    perror("Unable to create SSL context");
    ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
  }

  return ctx;
}

void configure_server_context(SSL_CTX *ctx) {
  /* Set the key and cert */
  if (SSL_CTX_use_certificate_chain_file(ctx, "cert.pem") <= 0) {
    ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
  }

  if (SSL_CTX_use_PrivateKey_file(ctx, "key.pem", SSL_FILETYPE_PEM) <= 0) {
    ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
  }
}

void configure_client_context(SSL_CTX *ctx) {
  /*
   * Configure the client to abort the handshake if certificate verification
   * fails
   */
  SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
  /*
   * In a real application you would probably just use the default system
   * certificate trust store and call: SSL_CTX_set_default_verify_paths(ctx); In
   * this demo though we are using a self-signed certificate, so the client must
   * trust it directly.
   */
  if (!SSL_CTX_load_verify_locations(ctx, "cert.pem", NULL)) {
    ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
  }
}
#endif /* 0 */
/* *****************************************************************************
OpenSSL Helpers Testing
***************************************************************************** */
#ifdef FIO_TEST_ALL
FIO_SFUNC void FIO_NAME_TEST(stl, openssl)(void) {
  /*
   * TODO: test module here
   */
}

#endif /* FIO_TEST_ALL */
/* *****************************************************************************
OpenSSL Helpers Cleanup
***************************************************************************** */

#endif /* FIO_EXTERN_COMPLETE */
#endif /* HAVE_OPENSSL */
