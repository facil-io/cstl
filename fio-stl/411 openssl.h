/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_IO                 /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                  OpenSSL Implementation for IO Functions




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(H___FIO_IO___H) && !defined(FIO_NO_TLS) &&                         \
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
#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>

/* *****************************************************************************
Validate OpenSSL Library Version
***************************************************************************** */

#if !defined(OPENSSL_VERSION_MAJOR) || OPENSSL_VERSION_MAJOR < 3
#undef HAVE_OPENSSL
#warning HAVE_OPENSSL flag error - incompatible OpenSSL version
/* No valid OpenSSL, return the default TLS IO functions */
SFUNC fio_io_functions_s fio_openssl_io_functions(void) {
  return fio_io_tls_default_functions(NULL);
}
#else
FIO_ASSERT_STATIC(OPENSSL_VERSION_MAJOR > 2, "OpenSSL version mismatch");

/* *****************************************************************************
Self-Signed Certificates using ECDSA P-256

Security notes:
- ECDSA P-256 provides 128-bit security (equivalent to RSA-3072)
- Key generation is ~200x faster than RSA-4096 (~10ms vs ~2000ms)
- Smaller certificates reduce TLS handshake overhead
- SHA-256 is used for signing (matched to P-256 security level)
- 180-day validity balances security with operational convenience
- Random 128-bit serial numbers prevent prediction attacks
- X.509v3 extensions ensure browser compatibility
***************************************************************************** */

/* Global ECDSA private key for self-signed certificates */
static EVP_PKEY *fio___openssl_pkey = NULL;

/* Cleanup callback to free the private key at exit */
static void fio___openssl_clear_root_key(void *key) {
  if (key) {
    EVP_PKEY_free((EVP_PKEY *)key);
  }
  fio___openssl_pkey = NULL;
}

/*
 * Generate an ECDSA P-256 private key for self-signed certificates.
 * Thread-safe with lock protection.
 *
 * Why ECDSA P-256:
 * - 128-bit security level (equivalent to RSA-3072)
 * - Key generation: ~10ms (vs ~2000ms for RSA-4096)
 * - Smaller keys: 256 bits (vs 4096 bits for RSA)
 * - NIST approved, widely supported
 * - Better performance for TLS handshakes
 */
static void fio___openssl_make_root_key(void) {
  static fio_lock_i lock = FIO_LOCK_INIT;
  fio_lock(&lock);
  if (!fio___openssl_pkey) {
    FIO_LOG_DEBUG2("generating ECDSA P-256 private key for TLS...");

    /* Create ECDSA P-256 key using modern OpenSSL 3.x API */
    fio___openssl_pkey = EVP_EC_gen("P-256");

    if (!fio___openssl_pkey) {
      /* Log the OpenSSL error */
      unsigned long err = ERR_get_error();
      char err_buf[256];
      ERR_error_string_n(err, err_buf, sizeof(err_buf));
      FIO_LOG_ERROR("OpenSSL ECDSA P-256 key generation failed: %s", err_buf);
      FIO_ASSERT(0, "OpenSSL failed to create ECDSA private key.");
    }

    /* Register cleanup callback */
    fio_state_callback_add(FIO_CALL_AT_EXIT,
                           fio___openssl_clear_root_key,
                           fio___openssl_pkey);
    FIO_LOG_DEBUG2("ECDSA P-256 private key generated successfully.");
  }
  fio_unlock(&lock);
}

/*
 * Add X.509v3 extensions to a certificate.
 *
 * Extensions added:
 * - Subject Alternative Name (SAN): Required by modern browsers
 * - Basic Constraints: CA:FALSE (not a CA certificate)
 * - Key Usage: digitalSignature, keyEncipherment (for TLS)
 * - Extended Key Usage: serverAuth (for HTTPS/TLS servers)
 *
 * Returns 0 on success, -1 on failure.
 */
FIO_SFUNC int fio___openssl_add_x509v3_extensions(X509 *cert,
                                                  const char *server_name) {
  X509V3_CTX ctx;
  X509_EXTENSION *ext = NULL;
  int ret = -1;

  /* Initialize extension context */
  X509V3_set_ctx_nodb(&ctx);
  X509V3_set_ctx(&ctx, cert, cert, NULL, NULL, 0);

  /* Basic Constraints: CA:FALSE - this is not a CA certificate */
  ext = X509V3_EXT_conf_nid(NULL, &ctx, NID_basic_constraints, "CA:FALSE");
  if (!ext) {
    FIO_LOG_ERROR("OpenSSL: failed to create Basic Constraints extension");
    goto cleanup;
  }
  if (!X509_add_ext(cert, ext, -1)) {
    FIO_LOG_ERROR("OpenSSL: failed to add Basic Constraints extension");
    X509_EXTENSION_free(ext);
    goto cleanup;
  }
  X509_EXTENSION_free(ext);
  ext = NULL;

  /* Key Usage: digitalSignature, keyEncipherment */
  ext = X509V3_EXT_conf_nid(NULL,
                            &ctx,
                            NID_key_usage,
                            "critical,digitalSignature,keyEncipherment");
  if (!ext) {
    FIO_LOG_ERROR("OpenSSL: failed to create Key Usage extension");
    goto cleanup;
  }
  if (!X509_add_ext(cert, ext, -1)) {
    FIO_LOG_ERROR("OpenSSL: failed to add Key Usage extension");
    X509_EXTENSION_free(ext);
    goto cleanup;
  }
  X509_EXTENSION_free(ext);
  ext = NULL;

  /* Extended Key Usage: serverAuth */
  ext = X509V3_EXT_conf_nid(NULL, &ctx, NID_ext_key_usage, "serverAuth");
  if (!ext) {
    FIO_LOG_ERROR("OpenSSL: failed to create Extended Key Usage extension");
    goto cleanup;
  }
  if (!X509_add_ext(cert, ext, -1)) {
    FIO_LOG_ERROR("OpenSSL: failed to add Extended Key Usage extension");
    X509_EXTENSION_free(ext);
    goto cleanup;
  }
  X509_EXTENSION_free(ext);
  ext = NULL;

  /* Subject Alternative Name (SAN) - required by modern browsers */
  if (server_name && server_name[0]) {
    /* Build SAN value: DNS:hostname */
    char san_value[512];
    int san_len = snprintf(san_value, sizeof(san_value), "DNS:%s", server_name);
    if (san_len > 0 && (size_t)san_len < sizeof(san_value)) {
      ext = X509V3_EXT_conf_nid(NULL, &ctx, NID_subject_alt_name, san_value);
      if (!ext) {
        FIO_LOG_ERROR("OpenSSL: failed to create SAN extension");
        goto cleanup;
      }
      if (!X509_add_ext(cert, ext, -1)) {
        FIO_LOG_ERROR("OpenSSL: failed to add SAN extension");
        X509_EXTENSION_free(ext);
        goto cleanup;
      }
      X509_EXTENSION_free(ext);
      ext = NULL;
    }
  }

  ret = 0; /* success */

cleanup:
  return ret;
}

/*
 * Generate a cryptographically random 128-bit serial number.
 *
 * Security rationale:
 * - Sequential serial numbers are predictable and can leak information
 * - 128-bit random provides ~2^128 possible values (practically unique)
 * - Uses OpenSSL's CSPRNG which sources from /dev/urandom or equivalent
 * - Meets CAB Forum Baseline Requirements for serial number entropy
 *
 * Returns 0 on success, -1 on failure.
 */
FIO_SFUNC int fio___openssl_set_random_serial(X509 *cert) {
  BIGNUM *bn = NULL;
  ASN1_INTEGER *serial = NULL;
  int ret = -1;

  /* Generate 128-bit random number */
  bn = BN_new();
  if (!bn) {
    FIO_LOG_ERROR("OpenSSL: BN_new failed for serial number");
    goto cleanup;
  }

  /* Generate 128 random bits using OpenSSL's CSPRNG */
  if (!BN_rand(bn, 128, BN_RAND_TOP_ANY, BN_RAND_BOTTOM_ANY)) {
    FIO_LOG_ERROR("OpenSSL: BN_rand failed for serial number");
    goto cleanup;
  }

  /* Ensure the number is positive (required for X.509 serial numbers) */
  BN_set_negative(bn, 0);

  /* Convert to ASN1_INTEGER and set on certificate */
  serial = BN_to_ASN1_INTEGER(bn, NULL);
  if (!serial) {
    FIO_LOG_ERROR("OpenSSL: BN_to_ASN1_INTEGER failed");
    goto cleanup;
  }

  if (!X509_set_serialNumber(cert, serial)) {
    FIO_LOG_ERROR("OpenSSL: X509_set_serialNumber failed");
    goto cleanup;
  }

  ret = 0; /* success */

cleanup:
  if (serial)
    ASN1_INTEGER_free(serial);
  if (bn)
    BN_free(bn);
  return ret;
}

/*
 * Create a self-signed X.509 certificate for TLS.
 *
 * Certificate properties:
 * - Algorithm: ECDSA with P-256 curve
 * - Signature: SHA-256 (matched to P-256 security level)
 * - Validity: 180 days (15552000 seconds)
 * - Serial: 128-bit cryptographically random
 * - Extensions: Basic Constraints, Key Usage, Extended Key Usage, SAN
 *
 * The certificate is self-signed and suitable for development/testing.
 * For production, use properly issued certificates from a trusted CA.
 */
static X509 *fio_io_tls_create_self_signed(const char *server_name) {
  X509 *cert = NULL;
  X509_NAME *name = NULL;

  /* Validate server_name */
  if (!server_name || !server_name[0]) {
    server_name = "localhost";
  }

  /* Check server_name length to prevent overflow */
  size_t srv_name_len = FIO_STRLEN(server_name);
  if (srv_name_len > 255) {
    FIO_LOG_ERROR("server_name too long for certificate (max 255 bytes)");
    return NULL;
  }

  /* Ensure we have a private key */
  fio___openssl_make_root_key();
  if (!fio___openssl_pkey) {
    FIO_LOG_ERROR("No private key available for self-signed certificate");
    return NULL;
  }

  /* Allocate new X509 certificate structure */
  cert = X509_new();
  if (!cert) {
    FIO_LOG_ERROR("OpenSSL: X509_new failed to allocate certificate");
    return NULL;
  }

  /* Set certificate version to X.509v3 (version value is 0-indexed, so 2 = v3)
   */
  if (!X509_set_version(cert, 2)) {
    FIO_LOG_ERROR("OpenSSL: X509_set_version failed");
    goto error;
  }

  /* Set cryptographically random serial number */
  if (fio___openssl_set_random_serial(cert) != 0) {
    FIO_LOG_ERROR("OpenSSL: failed to set random serial number");
    goto error;
  }

  /*
   * Set validity period: 180 days
   * - notBefore: now
   * - notAfter: now + 180 days (15552000 seconds)
   *
   * Calculation: 180 days * 24 hours * 60 minutes * 60 seconds = 15552000
   */
  if (!X509_gmtime_adj(X509_get_notBefore(cert), 0)) {
    FIO_LOG_ERROR("OpenSSL: X509_gmtime_adj failed for notBefore");
    goto error;
  }
  if (!X509_gmtime_adj(X509_get_notAfter(cert), 15552000L)) {
    FIO_LOG_ERROR("OpenSSL: X509_gmtime_adj failed for notAfter");
    goto error;
  }

  /* Set the public key from our ECDSA key pair */
  if (!X509_set_pubkey(cert, fio___openssl_pkey)) {
    FIO_LOG_ERROR("OpenSSL: X509_set_pubkey failed");
    goto error;
  }

  /* Set subject name with Organization and Common Name */
  name = X509_get_subject_name(cert);
  if (!name) {
    FIO_LOG_ERROR("OpenSSL: X509_get_subject_name failed");
    goto error;
  }

  /* Add Organization (O) - identifies the certificate owner */
  if (!X509_NAME_add_entry_by_txt(name,
                                  "O",
                                  MBSTRING_ASC,
                                  (const unsigned char *)server_name,
                                  (int)srv_name_len,
                                  -1,
                                  0)) {
    FIO_LOG_ERROR("OpenSSL: failed to add Organization to certificate");
    goto error;
  }

  /* Add Common Name (CN) - the server's hostname */
  if (!X509_NAME_add_entry_by_txt(name,
                                  "CN",
                                  MBSTRING_ASC,
                                  (const unsigned char *)server_name,
                                  (int)srv_name_len,
                                  -1,
                                  0)) {
    FIO_LOG_ERROR("OpenSSL: failed to add Common Name to certificate");
    goto error;
  }

  /* Set issuer name (same as subject for self-signed) */
  if (!X509_set_issuer_name(cert, name)) {
    FIO_LOG_ERROR("OpenSSL: X509_set_issuer_name failed");
    goto error;
  }

  /* Add X.509v3 extensions for browser compatibility */
  if (fio___openssl_add_x509v3_extensions(cert, server_name) != 0) {
    FIO_LOG_ERROR("OpenSSL: failed to add X.509v3 extensions");
    goto error;
  }

  /*
   * Sign the certificate with SHA-256.
   * SHA-256 is matched to P-256's security level (both ~128-bit security).
   */
  if (!X509_sign(cert, fio___openssl_pkey, EVP_sha256())) {
    unsigned long err = ERR_get_error();
    char err_buf[256];
    ERR_error_string_n(err, err_buf, sizeof(err_buf));
    FIO_LOG_ERROR("OpenSSL: X509_sign failed: %s", err_buf);
    goto error;
  }

  FIO_LOG_DEBUG2("created self-signed certificate for '%s' (ECDSA P-256, "
                 "SHA-256, 180 days)",
                 server_name);
  return cert;

error:
  if (cert)
    X509_free(cert);
  return NULL;
}

/* *****************************************************************************
OpenSSL Context type wrappers
***************************************************************************** */

/* Context for all future connections */
typedef struct {
  SSL_CTX *ctx;
  fio_io_tls_s *tls;
} fio___openssl_context_s;

FIO_LEAK_COUNTER_DEF(fio___openssl_context_s)

/* *****************************************************************************
OpenSSL Callbacks
***************************************************************************** */

FIO_SFUNC int fio___openssl_pem_password_cb(char *buf,
                                            int size,
                                            int rwflag,
                                            void *u) {
  const char *password = (const char *)u;
  if (!password)
    return 0;
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
  fio_io_s *io = (fio_io_s *)SSL_get_ex_data(ssl, 0);
  fio___openssl_context_s *ctx = (fio___openssl_context_s *)tls_;

  const unsigned char *end = in + inlen;
  char buf[256];
  while (in < end) {
    uint8_t len = in[0];
    if (len == 0 || (size_t)in + 1 + len > (size_t)end)
      break;
    if (len < sizeof(buf) - 1) {
      FIO_MEMCPY(buf, in + 1, len);
      buf[len] = 0;
      if (!fio_io_tls_alpn_select(ctx->tls, buf, (size_t)len, io)) {
        *out = in + 1;
        *outlen = len;
        FIO_LOG_DDEBUG2("(%d) TLS ALPN set to: %s for %p",
                        (int)fio_thread_getpid(),
                        buf,
                        (void *)io);
        return SSL_TLSEXT_ERR_OK;
      }
    }
    in += len + 1;
  }
  FIO_LOG_DDEBUG2("(%d) ALPN Failed! No protocol name match for %p",
                  (int)fio_thread_getpid(),
                  (void *)io);
  return SSL_TLSEXT_ERR_ALERT_FATAL;
  (void)tls_;
}

/* *****************************************************************************
Public Context Builder
***************************************************************************** */

FIO_SFUNC int fio___openssl_each_cert(struct fio_io_tls_each_s *e,
                                      const char *server_name,
                                      const char *public_cert_file,
                                      const char *private_key_file,
                                      const char *pk_password) {
  fio___openssl_context_s *s = (fio___openssl_context_s *)e->udata;
  if (public_cert_file && private_key_file) { /* load certificate from files */
    SSL_CTX_set_default_passwd_cb(s->ctx, fio___openssl_pem_password_cb);
    SSL_CTX_set_default_passwd_cb_userdata(s->ctx, (void *)pk_password);
    FIO_LOG_DDEBUG2("loading TLS certificates: %s & %s",
                    public_cert_file,
                    private_key_file);
    /* Set the certificate */
    if (SSL_CTX_use_certificate_chain_file(s->ctx, public_cert_file) <= 0) {
      ERR_print_errors_fp(stderr);
      FIO_LOG_ERROR("OpenSSL couldn't load certificate file: %s",
                    public_cert_file);
      return -1;
    }
    /* Set the private key */
    if (SSL_CTX_use_PrivateKey_file(s->ctx,
                                    private_key_file,
                                    SSL_FILETYPE_PEM) <= 0) {
      ERR_print_errors_fp(stderr);
      FIO_LOG_ERROR("OpenSSL couldn't load private key file: %s",
                    private_key_file);
      return -1;
    }
    /* Verify key matches certificate */
    if (!SSL_CTX_check_private_key(s->ctx)) {
      FIO_LOG_ERROR("OpenSSL: private key doesn't match certificate");
      return -1;
    }
    SSL_CTX_set_default_passwd_cb(s->ctx, NULL);
    SSL_CTX_set_default_passwd_cb_userdata(s->ctx, NULL);
  } else { /* generate self-signed certificate */
    if (!server_name || !server_name[0])
      server_name = "localhost";
    X509 *cert = fio_io_tls_create_self_signed(server_name);
    if (!cert) {
      FIO_LOG_ERROR("failed to create self-signed certificate");
      return -1;
    }
    if (!SSL_CTX_use_certificate(s->ctx, cert)) {
      X509_free(cert);
      FIO_LOG_ERROR("OpenSSL: SSL_CTX_use_certificate failed");
      return -1;
    }
    X509_free(cert); /* SSL_CTX makes a copy */
    if (!SSL_CTX_use_PrivateKey(s->ctx, fio___openssl_pkey)) {
      FIO_LOG_ERROR("OpenSSL: SSL_CTX_use_PrivateKey failed");
      return -1;
    }
  }
  return 0;
}

FIO_SFUNC int fio___openssl_each_alpn(struct fio_io_tls_each_s *e,
                                      const char *protocol_name,
                                      void (*on_selected)(fio_io_s *)) {
  fio_str_info_s *str = (fio_str_info_s *)e->udata2;
  if (!protocol_name)
    return 0;
  size_t len = FIO_STRLEN(protocol_name);
  if (len == 0 || len > 255) {
    FIO_LOG_ERROR("ALPN protocol name invalid length: %zu", len);
    return -1;
  }
  if (len + str->len + 1 > str->capa) {
    FIO_LOG_ERROR("ALPN protocol list overflow.");
    return -1;
  }
  str->buf[str->len++] = (char)(len & 0xFF);
  FIO_MEMCPY(str->buf + str->len, protocol_name, len);
  str->len += len;
  return 0;
  (void)on_selected;
}

FIO_SFUNC int fio___openssl_each_trust(struct fio_io_tls_each_s *e,
                                       const char *public_cert_file) {
  X509_STORE *store = (X509_STORE *)e->udata2;
  if (!store)
    return -1;
  if (public_cert_file) { /* trust specific certificate */
    if (!X509_STORE_load_file(store, public_cert_file)) {
      FIO_LOG_ERROR("OpenSSL: failed to load trust certificate: %s",
                    public_cert_file);
      return -1;
    }
  } else { /* trust system's default trust store */
    const char *path = getenv(X509_get_default_cert_dir_env());
    if (!path)
      path = X509_get_default_cert_dir();
    if (path) {
      if (!X509_STORE_load_path(store, path)) {
        FIO_LOG_WARNING("OpenSSL: failed to load system trust store from: %s",
                        path);
      }
    }
  }
  return 0;
}

/** Helper that converts a `fio_io_tls_s` into the implementation's context. */
FIO_SFUNC void *fio___openssl_build_context(fio_io_tls_s *tls,
                                            uint8_t is_client) {
  fio___openssl_context_s *ctx =
      (fio___openssl_context_s *)FIO_MEM_REALLOC(NULL, 0, sizeof(*ctx), 0);
  if (!ctx) {
    FIO_LOG_ERROR("OpenSSL: memory allocation failed for context");
    return NULL;
  }
  FIO_LEAK_COUNTER_ON_ALLOC(fio___openssl_context_s);
  *ctx = (fio___openssl_context_s){
      .ctx = SSL_CTX_new((is_client ? TLS_client_method : TLS_server_method)()),
      .tls = fio_io_tls_dup(tls),
  };

  if (!ctx->ctx) {
    FIO_LOG_ERROR("OpenSSL: SSL_CTX_new failed");
    if (ctx->tls)
      fio_io_tls_free(ctx->tls);
    FIO_LEAK_COUNTER_ON_FREE(fio___openssl_context_s);
    FIO_MEM_FREE(ctx, sizeof(*ctx));
    return NULL;
  }

  /* Configure SSL context modes for non-blocking I/O */
  SSL_CTX_set_mode(ctx->ctx, SSL_MODE_ENABLE_PARTIAL_WRITE);
  SSL_CTX_set_mode(ctx->ctx, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
  SSL_CTX_clear_mode(ctx->ctx, SSL_MODE_AUTO_RETRY);

  /* Configure certificate verification */
  X509_STORE *store = NULL;
  if (fio_io_tls_trust_count(tls)) {
    SSL_CTX_set_verify(ctx->ctx, SSL_VERIFY_PEER, NULL);
    store = X509_STORE_new();
    if (!store) {
      FIO_LOG_ERROR("OpenSSL: X509_STORE_new failed");
      goto error;
    }
    SSL_CTX_set_cert_store(ctx->ctx, store); /* takes ownership of store */
  } else {
    SSL_CTX_set_verify(ctx->ctx, SSL_VERIFY_NONE, NULL);
    if (is_client)
      FIO_LOG_SECURITY("no trusted TLS certificates listed for client, using "
                       "SSL_VERIFY_NONE!");
  }

  /* Load certificates or generate self-signed */
  if (!fio_io_tls_cert_count(tls) && !is_client) {
    /* No certificates configured for server - use self-signed */
    X509 *cert = fio_io_tls_create_self_signed("localhost");
    if (!cert) {
      FIO_LOG_ERROR("OpenSSL: failed to create self-signed certificate");
      goto error;
    }
    if (!SSL_CTX_use_certificate(ctx->ctx, cert)) {
      X509_free(cert);
      FIO_LOG_ERROR("OpenSSL: SSL_CTX_use_certificate failed");
      goto error;
    }
    X509_free(cert); /* SSL_CTX makes a copy */
    if (!SSL_CTX_use_PrivateKey(ctx->ctx, fio___openssl_pkey)) {
      FIO_LOG_ERROR("OpenSSL: SSL_CTX_use_PrivateKey failed");
      goto error;
    }
  }

  /* Process each configured certificate */
  if (fio_io_tls_each(tls,
                      .udata = ctx,
                      .udata2 = store,
                      .each_cert = fio___openssl_each_cert,
                      .each_trust = fio___openssl_each_trust)) {
    FIO_LOG_ERROR("OpenSSL: failed to configure certificates or trust store");
    goto error;
  }

  /* Configure ALPN if protocols are registered */
  if (fio_io_tls_alpn_count(tls)) {
    FIO_STR_INFO_TMP_VAR(alpn_list, 1023);
    if (fio_io_tls_each(tls,
                        .udata = ctx,
                        .udata2 = &alpn_list,
                        .each_alpn = fio___openssl_each_alpn)) {
      FIO_LOG_ERROR("OpenSSL: failed to configure ALPN protocols");
      goto error;
    }
    if (alpn_list.len > 0) {
      if (SSL_CTX_set_alpn_protos(ctx->ctx,
                                  (const unsigned char *)alpn_list.buf,
                                  (unsigned int)alpn_list.len)) {
        FIO_LOG_ERROR("SSL_CTX_set_alpn_protos failed.");
        /* ALPN is optional, continue without it */
      } else {
        SSL_CTX_set_alpn_select_cb(ctx->ctx,
                                   fio___openssl_alpn_selector_cb,
                                   ctx);
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
  }
  return ctx;

error:
  if (ctx) {
    if (ctx->ctx)
      SSL_CTX_free(ctx->ctx);
    if (ctx->tls)
      fio_io_tls_free(ctx->tls);
    FIO_LEAK_COUNTER_ON_FREE(fio___openssl_context_s);
    FIO_MEM_FREE(ctx, sizeof(*ctx));
  }
  return NULL;
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
  if (!ssl || !buf)
    return -1;
  errno = 0;
  if (len > INT_MAX)
    len = INT_MAX;
  r = SSL_read(ssl, buf, (int)len);
  if (r > 0)
    return r;
  if (errno == EWOULDBLOCK || errno == EAGAIN)
    return (ssize_t)-1;

  switch ((r = (ssize_t)SSL_get_error(ssl, (int)r))) {
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
#ifdef SSL_ERROR_WANT_ASYNC
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
  if (len > INT_MAX)
    len = INT_MAX;
  r = SSL_write(ssl, buf, (int)len);
  if (r > 0)
    return r;
  if (errno == EWOULDBLOCK || errno == EAGAIN)
    return -1;

  switch ((r = (ssize_t)SSL_get_error(ssl, (int)r))) {
  case SSL_ERROR_SSL:                         /* fall through */
  case SSL_ERROR_SYSCALL:                     /* fall through */
  case SSL_ERROR_ZERO_RETURN: return (r = 0); /* EOF */
  case SSL_ERROR_NONE:                        /* fall through */
  case SSL_ERROR_WANT_CONNECT:                /* fall through */
  case SSL_ERROR_WANT_ACCEPT:                 /* fall through */
  case SSL_ERROR_WANT_X509_LOOKUP:            /* fall through */
  case SSL_ERROR_WANT_WRITE:                  /* fall through */
  case SSL_ERROR_WANT_READ:                   /* fall through */
#ifdef SSL_ERROR_WANT_ASYNC /* fall through */
  case SSL_ERROR_WANT_ASYNC:                  /* fall through */
#endif
  default: errno = EWOULDBLOCK; return (r = -1);
  }
  (void)fd;
}

/* *****************************************************************************
Per-Connection Builder
***************************************************************************** */

FIO_LEAK_COUNTER_DEF(fio___SSL)

/** called once the IO was attached and the TLS object was set. */
FIO_SFUNC void fio___openssl_start(fio_io_s *io) {
  fio___openssl_context_s *ctx_parent =
      (fio___openssl_context_s *)fio_io_tls(io);
  if (!ctx_parent || !ctx_parent->ctx) {
    FIO_LOG_ERROR("OpenSSL Context missing for connection!");
    return;
  }

  SSL *ssl = SSL_new(ctx_parent->ctx);
  if (!ssl) {
    FIO_LOG_ERROR("OpenSSL: SSL_new failed");
    return;
  }
  FIO_LEAK_COUNTER_ON_ALLOC(fio___SSL);
  fio_io_tls_set(io, (void *)ssl);

  /* attach socket */
  FIO_LOG_DDEBUG2("(%d) allocated new TLS context for %p.",
                  (int)fio_thread_getpid(),
                  (void *)io);
  BIO *bio = BIO_new_socket(fio_io_fd(io), 0);
  if (!bio) {
    FIO_LOG_ERROR("OpenSSL: BIO_new_socket failed");
    FIO_LEAK_COUNTER_ON_FREE(fio___SSL);
    SSL_free(ssl);
    fio_io_tls_set(io, NULL);
    return;
  }
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

/** Called when the IO object finished sending all data before closure. */
FIO_SFUNC void fio___openssl_finish(int fd, void *tls_ctx) {
  SSL *ssl = (SSL *)tls_ctx;
  if (ssl)
    SSL_shutdown(ssl);
  (void)fd;
}

/* *****************************************************************************
Per-Connection Cleanup
***************************************************************************** */

/** Called after the IO object is closed, used to cleanup its `tls` object. */
FIO_SFUNC void fio___openssl_cleanup(void *tls_ctx) {
  SSL *ssl = (SSL *)tls_ctx;
  if (ssl) {
    SSL_shutdown(ssl);
    FIO_LEAK_COUNTER_ON_FREE(fio___SSL);
    SSL_free(ssl);
  }
}

/* *****************************************************************************
Context Cleanup
***************************************************************************** */

static void fio___openssl_free_context_task(void *tls_ctx, void *ignr_) {
  fio___openssl_context_s *ctx = (fio___openssl_context_s *)tls_ctx;
  if (!ctx)
    return;
  FIO_LEAK_COUNTER_ON_FREE(fio___openssl_context_s);
  if (ctx->ctx)
    SSL_CTX_free(ctx->ctx);
  if (ctx->tls)
    fio_io_tls_free(ctx->tls);
  FIO_MEM_FREE(ctx, sizeof(*ctx));
  (void)ignr_;
}

/** Builds a local TLS context out of the fio_io_tls_s object. */
static void fio___openssl_free_context(void *tls_ctx) {
  fio_io_defer(fio___openssl_free_context_task, tls_ctx, NULL);
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
      .finish = fio___openssl_finish,
  };
}

/* Setup OpenSSL as TLS IO default */
FIO_CONSTRUCTOR(fio___openssl_setup_default) {
  static fio_io_functions_s FIO___OPENSSL_IO_FUNCS;
  FIO___OPENSSL_IO_FUNCS = fio_openssl_io_functions();
  fio_io_tls_default_functions(&FIO___OPENSSL_IO_FUNCS);
#ifdef SIGPIPE
  fio_signal_monitor(.sig = SIGPIPE); /* avoid OpenSSL issue... */
#endif
}

/* *****************************************************************************
OpenSSL Helpers Cleanup
***************************************************************************** */
#endif /* defined(OPENSSL_VERSION_MAJOR) && OPENSSL_VERSION_MAJOR >= 3 */
#endif /* FIO_EXTERN_COMPLETE */
#endif /* HAVE_OPENSSL */
