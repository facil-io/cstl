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
#if defined(H___FIO_SERVER___H) &&                                             \
    (HAVE_OPENSSL || __has_include("openssl/ssl.h")) &&                        \
     !defined(H___FIO_OPENSSL___H) && !defined(FIO___RECURSIVE_INCLUDE)
#define H___FIO_OPENSSL___H 1
/* *****************************************************************************
OpenSSL IO Function Getter
***************************************************************************** */

/* Returns the OpenSSL IO functions. */
SFUNC fio_io_functions_s fio_openssl_io_functions(void);

/* *****************************************************************************
OpenSSL Helpers Implementation
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

#include <openssl/err.h>
#include <openssl/ssl.h>

FIO_ASSERT_STATIC(OPENSSL_VERSION_MAJOR > 2, "OpenSSL version mismatch");

/* *****************************************************************************
Self-Signed Certificates - TODO: change to ECDSA
***************************************************************************** */
static EVP_PKEY *fio___openssl_pkey = NULL;
static void fio___openssl_clear_root_key(void *key) {
  EVP_PKEY_free(((EVP_PKEY *)key));
  fio___openssl_pkey = NULL;
}

static void fio___openssl_make_root_key(void) {
  static fio_lock_i lock = FIO_LOCK_INIT;
  fio_lock(&lock);
  if (!fio___openssl_pkey) {
    /* create private key, free it at exit */
    FIO_LOG_DEBUG2("calculating a new TLS private key... might take a while.");
    fio___openssl_pkey = EVP_RSA_gen(2048);
    FIO_ASSERT(fio___openssl_pkey, "OpenSSL failed to create private key.");
    fio_state_callback_add(FIO_CALL_AT_EXIT,
                           fio___openssl_clear_root_key,
                           fio___openssl_pkey);
  }
  fio_unlock(&lock);
}

static X509 *fio_tls_create_self_signed(const char *server_name) {
  X509 *cert = X509_new();
  static uint32_t counter = 0;
  FIO_ASSERT(cert,
             "OpenSSL failed to allocate memory for self-signed certificate.");
  fio___openssl_make_root_key();

  /* serial number */
  fio_atomic_add(&counter, 1);
  ASN1_INTEGER_set(X509_get_serialNumber(cert), counter);

  /* validity (180 days) */
  X509_gmtime_adj(X509_get_notBefore(cert), 0);
  X509_gmtime_adj(X509_get_notAfter(cert), 15552000L);

  /* set (public) key */
  X509_set_pubkey(cert, fio___openssl_pkey);

  /* set identity details */
  X509_NAME *s = X509_get_subject_name(cert);
  size_t srv_name_len = FIO_STRLEN(server_name);
  X509_NAME_add_entry_by_txt(s,
                             "O",
                             MBSTRING_ASC,
                             (unsigned char *)server_name,
                             srv_name_len,
                             -1,
                             0);
  X509_NAME_add_entry_by_txt(s,
                             "CN",
                             MBSTRING_ASC,
                             (unsigned char *)server_name,
                             srv_name_len,
                             -1,
                             0);
  X509_NAME_add_entry_by_txt(s,
                             "CA",
                             MBSTRING_ASC,
                             (unsigned char *)server_name,
                             srv_name_len,
                             -1,
                             0);
  X509_set_issuer_name(cert, s);

  /* sign certificate */
  FIO_ASSERT(X509_sign(cert, fio___openssl_pkey, EVP_sha512()),
             "OpenSSL failed to signed self-signed certificate");
  return cert;
}

/* *****************************************************************************
OpenSSL Context type wrappers
***************************************************************************** */

/* Context for all future connections */
typedef struct {
  SSL_CTX *ctx;
  fio_tls_s *tls;
} fio___openssl_context_s;

FIO___LEAK_COUNTER_DEF(fio___openssl_context_s)

/* *****************************************************************************
OpenSSL Callbacks
***************************************************************************** */

FIO_SFUNC int fio___openssl_pem_password_cb(char *buf,
                                            int size,
                                            int rwflag,
                                            void *u) {
  const char *password = (const char *)u;
  size_t password_len = FIO_STRLEN(password);
  if (password_len > (size_t)size)
    return -1;
  FIO_MEMCPY(buf, password, password_len);
  return (int)password_len;
  (void)rwflag;
}

FIO_SFUNC int fio___openssl_alpn_selector_cb(SSL *ssl,
                                             const unsigned char **out,
                                             unsigned char *outlen,
                                             const unsigned char *in,
                                             unsigned int inlen,
                                             void *tls_) {
  fio_s *io = (fio_s *)SSL_get_ex_data(ssl, 0);
  fio___openssl_context_s *ctx = (fio___openssl_context_s *)tls_;

  const unsigned char *end = in + inlen;
  char buf[256];
  for (;;) {
    uint8_t len = in[0];
    FIO_MEMCPY(buf, in + 1, len);
    buf[len] = 0;
    if (fio_tls_alpn_select(ctx->tls, buf, (size_t)len, io)) {
      in += len + 1;
      if (in < end)
        continue;
      FIO_LOG_DDEBUG2("(%d) ALPN Failed! No protocol name match for %p",
                      (int)fio_thread_getpid(),
                      io);
      return SSL_TLSEXT_ERR_ALERT_FATAL;
    }
    *out = in + 1;
    *outlen = len;
    FIO_LOG_DDEBUG2("(%d) TLS ALPN set to: %s for %p",
                    (int)fio_thread_getpid(),
                    buf,
                    io);
    return SSL_TLSEXT_ERR_OK;
    (void)tls_;
  }
}

/* *****************************************************************************
Public Context Builder
***************************************************************************** */

FIO_SFUNC int fio___openssl_each_cert(struct fio_tls_each_s *e,
                                      const char *server_name,
                                      const char *public_cert_file,
                                      const char *private_key_file,
                                      const char *pk_password) {
  fio___openssl_context_s *s = (fio___openssl_context_s *)e->udata;
  if (public_cert_file && private_key_file) { /* load certificate */
    SSL_CTX_set_default_passwd_cb(s->ctx, fio___openssl_pem_password_cb);
    SSL_CTX_set_default_passwd_cb_userdata(s->ctx, (void *)pk_password);
    FIO_LOG_DDEBUG2("loading TLS certificates: %s & %s",
                    public_cert_file,
                    private_key_file);
    /* Set the key and cert */
    if (SSL_CTX_use_certificate_chain_file(s->ctx, public_cert_file) <= 0) {
      ERR_print_errors_fp(stderr);
      FIO_ASSERT(0,
                 "OpenSSL couldn't open PEM file for certificate: %s",
                 public_cert_file);
    }

    if (SSL_CTX_use_PrivateKey_file(s->ctx,
                                    private_key_file,
                                    SSL_FILETYPE_PEM) <= 0) {
      ERR_print_errors_fp(stderr);
      FIO_ASSERT(0,
                 "OpenSSL couldn't open PEM file for private key: %s",
                 private_key_file);
    }
    SSL_CTX_set_default_passwd_cb(s->ctx, NULL);
    SSL_CTX_set_default_passwd_cb_userdata(s->ctx, NULL);
  } else { /* self signed */
    if (!server_name || !strlen(server_name))
      server_name = (const char *)"localhost";
    X509 *cert = fio_tls_create_self_signed(server_name);
    SSL_CTX_use_certificate(s->ctx, cert);
    SSL_CTX_use_PrivateKey(s->ctx, fio___openssl_pkey);
  }
  return 0;
}

FIO_SFUNC int fio___openssl_each_alpn(struct fio_tls_each_s *e,
                                      const char *protocol_name,
                                      void (*on_selected)(fio_s *)) {
  fio_str_info_s *str = (fio_str_info_s *)e->udata2;
  size_t len = FIO_STRLEN(protocol_name);
  if (len > 255 || (len + str->len >= str->capa)) {
    FIO_LOG_ERROR("ALPN protocol name/list overflow.");
    return -1;
  }
  str->buf[str->len++] = (len & 0xFF);
  FIO_MEMCPY(str->buf + str->len, protocol_name, len);
  str->len += len;
  return 0;
  (void)on_selected;
}

FIO_SFUNC int fio___openssl_each_trust(struct fio_tls_each_s *e,
                                       const char *public_cert_file) {
  X509_STORE *store = (X509_STORE *)e->udata2;
  if (public_cert_file) /* trust specific certificate */
    X509_STORE_load_file(store, public_cert_file);
  else { /* trust system's trust */
    const char *path = getenv(X509_get_default_cert_dir_env());
    if (!path)
      path = X509_get_default_cert_dir();
    if (path)
      X509_STORE_load_path(store, path);
  }
  return 0;
}

/** Helper that converts a `fio_tls_s` into the implementation's context. */
FIO_SFUNC void *fio___openssl_build_context(fio_tls_s *tls, uint8_t is_client) {
  fio___openssl_context_s *ctx =
      (fio___openssl_context_s *)FIO_MEM_REALLOC(NULL, 0, sizeof(*ctx), 0);
  FIO_ASSERT_ALLOC(ctx);
  FIO___LEAK_COUNTER_ON_ALLOC(fio___openssl_context_s);
  *ctx = (fio___openssl_context_s){
      .ctx = SSL_CTX_new((is_client ? TLS_client_method : TLS_server_method)()),
      .tls = fio_tls_dup(tls),
  };

  SSL_CTX_set_mode(ctx->ctx, SSL_MODE_ENABLE_PARTIAL_WRITE);
  SSL_CTX_set_mode(ctx->ctx, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
  SSL_CTX_clear_mode(ctx->ctx, SSL_MODE_AUTO_RETRY);

  X509_STORE *store = NULL;
  if (fio_tls_trust_count(tls)) {
    SSL_CTX_set_verify(ctx->ctx, SSL_VERIFY_PEER, NULL);
    store = X509_STORE_new();
    SSL_CTX_set_cert_store(ctx->ctx, store);
  } else {
    SSL_CTX_set_verify(ctx->ctx, SSL_VERIFY_NONE, NULL);
  }
  if (!fio_tls_cert_count(tls) && !is_client) {
    /* add self-signed certificate to context */
    X509 *cert = fio_tls_create_self_signed("localhost");
    SSL_CTX_use_certificate(ctx->ctx, cert);
    SSL_CTX_use_PrivateKey(ctx->ctx, fio___openssl_pkey);
  }
  fio_tls_each(tls,
               .udata = ctx,
               .udata2 = store,
               .each_cert = fio___openssl_each_cert,
               .each_trust = fio___openssl_each_trust);

  if (fio_tls_alpn_count(tls)) {
    FIO_STR_INFO_TMP_VAR(alpn_list, 1023);
    fio_tls_each(tls,
                 .udata = ctx,
                 .udata2 = &alpn_list,
                 .each_alpn = fio___openssl_each_alpn);
    if (SSL_CTX_set_alpn_protos(ctx->ctx,
                                (const unsigned char *)alpn_list.buf,
                                (unsigned int)alpn_list.len)) {
      FIO_LOG_ERROR("SSL_CTX_set_alpn_protos failed.");
    } else {
      SSL_CTX_set_alpn_select_cb(ctx->ctx, fio___openssl_alpn_selector_cb, ctx);
      SSL_CTX_set_next_proto_select_cb(
          ctx->ctx,
          (int (*)(SSL *,
                   unsigned char **,
                   unsigned char *,
                   const unsigned char *,
                   unsigned int,
                   void *))fio___openssl_alpn_selector_cb,
          (void *)ctx);
    }
  }
  return ctx;
}

/* *****************************************************************************
IO functions
***************************************************************************** */

/** Called to perform a non-blocking `read`, same as the system call. */
FIO_SFUNC ssize_t fio___openssl_read(int fd,
                                     void *buf,
                                     size_t len,
                                     void *tls_ctx) {
  ssize_t r;
  SSL *ssl = (SSL *)tls_ctx;
  errno = 0;
  r = SSL_read(ssl, buf, len);
  if (r > 0)
    return r;
  if (errno == EWOULDBLOCK || errno == EAGAIN)
    return -1;

  switch ((r = SSL_get_error(ssl, r))) {
  case SSL_ERROR_SSL:                                   /* fall through */
  case SSL_ERROR_SYSCALL:                               /* fall through */
  case SSL_ERROR_ZERO_RETURN: return (r = 0);           /* EOF */
  case SSL_ERROR_NONE:                                  /* fall through */
  case SSL_ERROR_WANT_CONNECT:                          /* fall through */
  case SSL_ERROR_WANT_ACCEPT:                           /* fall through */
  case SSL_ERROR_WANT_WRITE:                            /* fall through */
    r = SSL_write_ex(ssl, (void *)&r, 0, (size_t *)&r); /* fall through */
  case SSL_ERROR_WANT_X509_LOOKUP:                      /* fall through */
  case SSL_ERROR_WANT_READ:                             /* fall through */
#ifdef SSL_ERROR_WANT_ASYNC                             /* fall through */
  case SSL_ERROR_WANT_ASYNC:                            /* fall through */
#endif
  default: errno = EWOULDBLOCK; return (r = -1);
  }
  (void)fd;
}

/** Sends any unsent internal data. Returns 0 only if all data was sent. */
FIO_SFUNC int fio___openssl_flush(int fd, void *tls_ctx) {
  return 0;
  (void)fd, (void)tls_ctx;
#if 0                       /* no flushing necessary? */
  int r;
  char buf[8] = {0};
  size_t count = 0;
  SSL *ssl = (SSL *)tls_ctx;
  r = SSL_write_ex(ssl, buf, 0, &count);
  if (count)
    return 1;
  if (r > 0)
    return 0;
  switch ((r = SSL_get_error(ssl, r))) {
  case SSL_ERROR_SSL:                         /* fall through */
  case SSL_ERROR_SYSCALL:                     /* fall through */
  case SSL_ERROR_NONE:                        /* fall through */
  case SSL_ERROR_ZERO_RETURN: return (r = 0); /* EOF */
  case SSL_ERROR_WANT_CONNECT:                /* fall through */
  case SSL_ERROR_WANT_ACCEPT:                 /* fall through */
  case SSL_ERROR_WANT_X509_LOOKUP:            /* fall through */
  case SSL_ERROR_WANT_READ:                   /* fall through */
    SSL_read_ex(ssl, buf, 0, &count);         /* fall through */
  case SSL_ERROR_WANT_WRITE:                  /* fall through */
#ifdef SSL_ERROR_WANT_ASYNC /* fall through */
  case SSL_ERROR_WANT_ASYNC:                  /* fall through */
#endif
  default: errno = EWOULDBLOCK; return -1;
  }
#endif
}

/** Called to perform a non-blocking `write`, same as the system call. */
FIO_SFUNC ssize_t fio___openssl_write(int fd,
                                      const void *buf,
                                      size_t len,
                                      void *tls_ctx) {
  ssize_t r = -1;
  if (!buf || !len || !tls_ctx)
    return r;
  SSL *ssl = (SSL *)tls_ctx;
  errno = 0;
  r = SSL_write(ssl, buf, len);
  if (r > 0)
    return r;
  if (errno == EWOULDBLOCK || errno == EAGAIN)
    return -1;

  switch ((r = SSL_get_error(ssl, r))) {
  case SSL_ERROR_SSL:                         /* fall through */
  case SSL_ERROR_SYSCALL:                     /* fall through */
  case SSL_ERROR_ZERO_RETURN: return (r = 0); /* EOF */
  case SSL_ERROR_NONE:                        /* fall through */
  case SSL_ERROR_WANT_CONNECT:                /* fall through */
  case SSL_ERROR_WANT_ACCEPT:                 /* fall through */
  case SSL_ERROR_WANT_X509_LOOKUP:            /* fall through */
  case SSL_ERROR_WANT_WRITE:                  /* fall through */
  case SSL_ERROR_WANT_READ:                   /* fall through */
#ifdef SSL_ERROR_WANT_ASYNC                   /* fall through */
  case SSL_ERROR_WANT_ASYNC:                  /* fall through */
#endif
  default: errno = EWOULDBLOCK; return (r = -1);
  }
  (void)fd;
}

/* *****************************************************************************
Per-Connection Builder
***************************************************************************** */

FIO___LEAK_COUNTER_DEF(fio___SSL)

/** called once the IO was attached and the TLS object was set. */
FIO_SFUNC void fio___openssl_start(fio_s *io) {
  fio___openssl_context_s *ctx_parent =
      (fio___openssl_context_s *)fio_tls_get(io);
  FIO_ASSERT_DEBUG(ctx_parent, "OpenSSL Context missing!");

  SSL *ssl = SSL_new(ctx_parent->ctx);
  FIO___LEAK_COUNTER_ON_ALLOC(fio___SSL);
  fio_tls_set(io, (void *)ssl);

  /* attach socket */
  FIO_LOG_DDEBUG2("(%d) allocated new TLS context for %p.",
                  (int)fio_thread_getpid(),
                  (void *)io);
  BIO *bio = BIO_new_socket(fio_fd_get(io), 0);
  SSL_set_bio(ssl, bio, bio);
  SSL_set_ex_data(ssl, 0, (void *)io);
  if (SSL_is_server(ssl))
    SSL_accept(ssl);
  else
    SSL_connect(ssl);
}

/* *****************************************************************************
Closing Connections
***************************************************************************** */

/** Decreases a fio_tls_s object's reference count, or frees the object. */
FIO_SFUNC void fio___openssl_finish(int fd, void *tls_ctx) {
  SSL *ssl = (SSL *)tls_ctx;
  SSL_shutdown(ssl);
  (void)fd;
}

/* *****************************************************************************
Per-Connection Cleanup
***************************************************************************** */

/** Decreases a fio_tls_s object's reference count, or frees the object. */
FIO_SFUNC void fio___openssl_cleanup(void *tls_ctx) {
  SSL *ssl = (SSL *)tls_ctx;
  SSL_shutdown(ssl);
  FIO___LEAK_COUNTER_ON_FREE(fio___SSL);
  SSL_free(ssl);
}

/* *****************************************************************************
Context Cleanup
***************************************************************************** */

static void fio___openssl_free_context_task(void *tls_ctx, void *ignr_) {
  fio___openssl_context_s *ctx = (fio___openssl_context_s *)tls_ctx;
  FIO___LEAK_COUNTER_ON_FREE(fio___openssl_context_s);
  SSL_CTX_free(ctx->ctx);
  fio_tls_free(ctx->tls);
  FIO_MEM_FREE(ctx, sizeof(*ctx));
  (void)ignr_;
}

/** Builds a local TLS context out of the fio_tls_s object. */
static void fio___openssl_free_context(void *tls_ctx) {
  fio_srv_defer(fio___openssl_free_context_task, tls_ctx, NULL);
}
/* *****************************************************************************
IO Functions Structure
***************************************************************************** */

/* Returns the OpenSSL IO functions (implementation) */
SFUNC fio_io_functions_s fio_openssl_io_functions(void) {
  return (fio_io_functions_s){
      .build_context = fio___openssl_build_context,
      .free_context = fio___openssl_free_context,
      .start = fio___openssl_start,
      .read = fio___openssl_read,
      .write = fio___openssl_write,
      .flush = fio___openssl_flush,
      .cleanup = fio___openssl_cleanup,
  };
}

/* Setup OpenSSL as TLS IO default */
FIO_CONSTRUCTOR(fio___openssl_setup_default) {
  static fio_io_functions_s FIO___OPENSSL_IO_FUNCS = {
      .build_context = fio___openssl_build_context,
      .free_context = fio___openssl_free_context,
      .start = fio___openssl_start,
      .read = fio___openssl_read,
      .write = fio___openssl_write,
      .flush = fio___openssl_flush,
      .cleanup = fio___openssl_cleanup,
  };
  fio_tls_default_io_functions(&FIO___OPENSSL_IO_FUNCS);
#ifdef SIGPIPE
  fio_signal_monitor(SIGPIPE, NULL, NULL); /* avoid OpenSSL issue... */
#endif
}

/* *****************************************************************************
OpenSSL Helpers Cleanup
***************************************************************************** */

#endif /* FIO_EXTERN_COMPLETE */
#endif /* HAVE_OPENSSL */
