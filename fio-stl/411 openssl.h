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

  /* Configure SSL context modes for non-blocking I/O.
   * PARTIAL_WRITE: allow SSL_write to return with partial data sent.
   * ACCEPT_MOVING_WRITE_BUFFER: buffer pointer may change between retries.
   * RELEASE_BUFFERS: free OpenSSL's internal 34KB per-connection read/write
   *   buffers when idle — dramatically reduces memory/cache pressure. */
  SSL_CTX_set_mode(ctx->ctx,
                   SSL_MODE_ENABLE_PARTIAL_WRITE |
                       SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER |
                       SSL_MODE_RELEASE_BUFFERS);
  SSL_CTX_clear_mode(ctx->ctx, SSL_MODE_AUTO_RETRY);

  /* Enable TLS 1.3 session tickets for resumption.
   * On reconnect, avoids full handshake — saves ~30-50% handshake CPU. */
  SSL_CTX_set_num_tickets(ctx->ctx, 2);

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
Per-Connection State (Custom BIO Architecture)

Instead of BIO_s_mem() (which copies data through internal BUF_MEM buffers),
we use custom BIO types that give OpenSSL direct access to our buffers:
- Custom rbio: OpenSSL reads raw socket data directly from conn->recv_buf
- Custom wbio: OpenSSL writes encrypted data directly into conn->enc_buf
This eliminates ALL intermediate copies, matching BIO_new_socket performance
while retaining full control over batching and partial write buffering.
***************************************************************************** */

/** Max encrypted record overhead: header(5) + max_plaintext(16384) + tag(16)
 *  + 256 bytes margin for TLS 1.2/1.3 variations and alignment. */
#define FIO___OPENSSL_ENC_RECORD_SIZE (16384 + 5 + 16 + 256)

/** Per-connection wrapper around SSL, with receive and encrypted output
 *  buffers. Custom BIOs read/write directly from/to these buffers. */
typedef struct {
  SSL *ssl;
  /* Cached handshake state — avoids SSL_is_init_finished() vtable call
   * on every read() in the hot path (post-handshake). */
  uint8_t handshake_complete;
  /* Receive buffer — raw socket data fed to OpenSSL for decryption.
   * We read the socket directly into this buffer, and OpenSSL's custom rbio
   * read callback pulls from it. Sized for one full IO read (64KB). */
  size_t recv_buf_len;
  size_t recv_buf_pos;
  uint8_t recv_buf[FIO_IO_BUFFER_PER_WRITE]; /* 64KB */
  /* Encrypted output buffer — OpenSSL's custom wbio write callback appends
   * directly here. Sized for 4 max TLS records (~66KB). */
  size_t enc_buf_len;
  size_t enc_buf_sent;
  uint8_t enc_buf[4 * FIO___OPENSSL_ENC_RECORD_SIZE];
} fio___openssl_connection_s;

FIO_LEAK_COUNTER_DEF(fio___openssl_connection_s)

/* *****************************************************************************
Custom BIO Methods — Write BIO (OpenSSL encrypted output → enc_buf)
***************************************************************************** */

/** Custom wbio write_ex: OpenSSL calls this to output encrypted data.
 *  Appends directly into conn->enc_buf — no intermediate BUF_MEM copy. */
FIO_SFUNC int fio___openssl_wbio_write_ex(BIO *bio,
                                          const char *data,
                                          size_t len,
                                          size_t *written) {
  fio___openssl_connection_s *conn =
      (fio___openssl_connection_s *)BIO_get_data(bio);
  if (!conn || !data || !len) {
    *written = 0;
    return 0;
  }
  size_t space = sizeof(conn->enc_buf) - conn->enc_buf_len;
  size_t to_write = (len < space) ? len : space;
  if (!to_write) {
    BIO_set_retry_write(bio);
    *written = 0;
    return 0;
  }
  FIO_MEMCPY(conn->enc_buf + conn->enc_buf_len, data, to_write);
  conn->enc_buf_len += to_write;
  *written = to_write;
  return 1;
}

/* *****************************************************************************
Custom BIO Methods — Read BIO (recv_buf → OpenSSL for decryption)
***************************************************************************** */

/** Custom rbio read_ex: OpenSSL calls this to get raw data for decryption.
 *  Reads directly from conn->recv_buf — no intermediate BUF_MEM copy. */
FIO_SFUNC int fio___openssl_rbio_read_ex(BIO *bio,
                                         char *buf,
                                         size_t len,
                                         size_t *readbytes) {
  fio___openssl_connection_s *conn =
      (fio___openssl_connection_s *)BIO_get_data(bio);
  if (!conn || !buf || !len) {
    *readbytes = 0;
    return 0;
  }
  size_t avail = conn->recv_buf_len - conn->recv_buf_pos;
  if (!avail) {
    BIO_set_retry_read(bio);
    *readbytes = 0;
    return 0;
  }
  size_t to_read = (len < avail) ? len : avail;
  FIO_MEMCPY(buf, conn->recv_buf + conn->recv_buf_pos, to_read);
  conn->recv_buf_pos += to_read;
  /* Compact when fully consumed */
  if (conn->recv_buf_pos >= conn->recv_buf_len) {
    conn->recv_buf_len = 0;
    conn->recv_buf_pos = 0;
  }
  *readbytes = to_read;
  return 1;
}

/* *****************************************************************************
Custom BIO Methods — Shared ctrl callback
***************************************************************************** */

/** ctrl callback for both custom BIOs. Handles flush, pending queries. */
FIO_SFUNC long fio___openssl_bio_ctrl(BIO *bio, int cmd, long num, void *ptr) {
  switch (cmd) {
  case BIO_CTRL_FLUSH: return 1; /* always "flushed" — we manage flushing */
  case BIO_CTRL_PUSH:            /* fall through */
  case BIO_CTRL_POP: return 0;
  case BIO_CTRL_WPENDING: {
    fio___openssl_connection_s *conn =
        (fio___openssl_connection_s *)BIO_get_data(bio);
    return conn ? (long)(conn->enc_buf_len - conn->enc_buf_sent) : 0;
  }
  case BIO_CTRL_PENDING: {
    fio___openssl_connection_s *conn =
        (fio___openssl_connection_s *)BIO_get_data(bio);
    return conn ? (long)(conn->recv_buf_len - conn->recv_buf_pos) : 0;
  }
  default: return 0;
  }
  (void)num;
  (void)ptr;
}

/* *****************************************************************************
Custom BIO Methods — Global BIO_METHOD objects (created once, read-only)
***************************************************************************** */

static BIO_METHOD *fio___openssl_rbio_method = NULL;
static BIO_METHOD *fio___openssl_wbio_method = NULL;

/** Initialize custom BIO methods. Thread-safe via lock. Call from constructor.
 */
FIO_SFUNC void fio___openssl_init_bio_methods(void) {
  static fio_lock_i lock = FIO_LOCK_INIT;
  fio_lock(&lock);
  if (!fio___openssl_rbio_method) {
    fio___openssl_rbio_method =
        BIO_meth_new(BIO_TYPE_SOURCE_SINK | BIO_get_new_index(), "fio_rbio");
    BIO_meth_set_read_ex(fio___openssl_rbio_method, fio___openssl_rbio_read_ex);
    BIO_meth_set_ctrl(fio___openssl_rbio_method, fio___openssl_bio_ctrl);

    fio___openssl_wbio_method =
        BIO_meth_new(BIO_TYPE_SOURCE_SINK | BIO_get_new_index(), "fio_wbio");
    BIO_meth_set_write_ex(fio___openssl_wbio_method,
                          fio___openssl_wbio_write_ex);
    BIO_meth_set_ctrl(fio___openssl_wbio_method, fio___openssl_bio_ctrl);
  }
  fio_unlock(&lock);
}

/** Cleanup callback to free BIO methods at exit. */
FIO_SFUNC void fio___openssl_free_bio_methods(void *ignr_) {
  if (fio___openssl_rbio_method) {
    BIO_meth_free(fio___openssl_rbio_method);
    fio___openssl_rbio_method = NULL;
  }
  if (fio___openssl_wbio_method) {
    BIO_meth_free(fio___openssl_wbio_method);
    fio___openssl_wbio_method = NULL;
  }
  (void)ignr_;
}

/* *****************************************************************************
Per-Connection Helpers
***************************************************************************** */

/** Try to send pending enc_buf data directly to socket.
 *  Returns total bytes sent, 0 if nothing pending or socket full. */
FIO_SFUNC size_t fio___openssl_send_enc_buf(int fd,
                                            fio___openssl_connection_s *conn) {
  if (conn->enc_buf_sent >= conn->enc_buf_len)
    return 0;

  /* Fast path: try to send everything in one call (common case) */
  size_t remaining = conn->enc_buf_len - conn->enc_buf_sent;
  ssize_t written =
      fio_sock_write(fd, (char *)conn->enc_buf + conn->enc_buf_sent, remaining);
  if (written > 0 && (size_t)written >= remaining) {
    /* All sent in one call */
    conn->enc_buf_len = 0;
    conn->enc_buf_sent = 0;
    return remaining;
  }
  if (written > 0)
    conn->enc_buf_sent += (size_t)written;

  /* Slow path: partial send, retry loop */
  size_t total = (written > 0) ? (size_t)written : 0;
  while (conn->enc_buf_sent < conn->enc_buf_len) {
    written = fio_sock_write(fd,
                             (char *)conn->enc_buf + conn->enc_buf_sent,
                             conn->enc_buf_len - conn->enc_buf_sent);
    if (written <= 0)
      break;
    conn->enc_buf_sent += (size_t)written;
    total += (size_t)written;
  }
  if (conn->enc_buf_sent >= conn->enc_buf_len) {
    conn->enc_buf_len = 0;
    conn->enc_buf_sent = 0;
  }
  return total;
}

/* *****************************************************************************
IO functions
***************************************************************************** */

/** Called to perform a non-blocking `read`, same as the system call.
 *
 * Custom BIO flow (one copy, same as BIO_new_socket):
 * 1. Read raw bytes from socket directly into conn->recv_buf (ONE copy)
 * 2. If handshake incomplete: call SSL_do_handshake() — OpenSSL pulls from
 *    recv_buf via custom rbio, writes handshake output to enc_buf via custom
 *    wbio. Send enc_buf immediately (IO layer won't call on_ready during
 *    handshake because there's no outgoing stream data yet).
 * 3. If handshake complete: call SSL_read_ex() to decrypt application data.
 *    OpenSSL pulls from recv_buf via custom rbio.
 * 4. Send any post-read output from enc_buf (key updates, etc.)
 */
FIO_SFUNC ssize_t fio___openssl_read(int fd,
                                     void *buf,
                                     size_t len,
                                     void *tls_ctx) {
  fio___openssl_connection_s *conn = (fio___openssl_connection_s *)tls_ctx;
  if (!conn || !buf)
    return -1;
  SSL *ssl = conn->ssl;

  /* Step 1: Compact recv_buf when >50% consumed, to maximize socket read space.
   * Without compaction, the buffer fragments as OpenSSL reads one record at a
   * time — recv_buf_pos advances but unconsumed data stays in place, leaving
   * shrinking space for new socket reads. */
  if (conn->recv_buf_pos > (sizeof(conn->recv_buf) >> 1)) {
    size_t remaining = conn->recv_buf_len - conn->recv_buf_pos;
    if (remaining > 0)
      FIO_MEMMOVE(conn->recv_buf,
                  conn->recv_buf + conn->recv_buf_pos,
                  remaining);
    conn->recv_buf_len = remaining;
    conn->recv_buf_pos = 0;
  }

  /* Step 2: Read raw data from socket directly into recv_buf.
   * ONE copy: socket → recv_buf. Same as BIO_new_socket. */
  if (conn->recv_buf_len < sizeof(conn->recv_buf)) {
    ssize_t raw = fio_sock_read(fd,
                                (char *)conn->recv_buf + conn->recv_buf_len,
                                sizeof(conn->recv_buf) - conn->recv_buf_len);
    if (raw > 0)
      conn->recv_buf_len += (size_t)raw;
    else if (raw == 0)
      return 0; /* EOF */
    /* raw == -1 with EWOULDBLOCK is OK — recv_buf may still have data
     * from a previous read that SSL_read can process. */
  }

  /* Step 3: If handshake not complete, advance it.
   * With custom BIOs, handshake responses land directly in enc_buf via the
   * wbio write callback. We MUST send them from within the read callback
   * because the IO layer's on_ready loop won't run until the protocol layer
   * queues outgoing data — which won't happen until after the handshake
   * completes. This matches the native TLS 1.3 implementation. */
  if (!conn->handshake_complete) {
    SSL_do_handshake(ssl);
    /* Handshake output is already in enc_buf — send it immediately */
    if (conn->enc_buf_len > conn->enc_buf_sent)
      fio___openssl_send_enc_buf(fd, conn);
    /* If handshake still not finished, tell IO layer to wait for more data */
    if (!SSL_is_init_finished(ssl)) {
      errno = EWOULDBLOCK;
      return -1;
    }
    /* Handshake just completed — cache the result to avoid vtable call */
    conn->handshake_complete = 1;
    /* Fall through to try SSL_read_ex */
  }

  /* Step 4: Decrypt application data.
   * OpenSSL pulls raw data from recv_buf via custom rbio read callback. */
  size_t readbytes = 0;
  int r = SSL_read_ex(ssl, buf, len, &readbytes);

  /* Step 5: Send any post-read output (key updates, renegotiation).
   * These are already in enc_buf via the custom wbio write callback. */
  if (conn->enc_buf_len > conn->enc_buf_sent)
    fio___openssl_send_enc_buf(fd, conn);

  if (r == 1 && readbytes > 0)
    return (ssize_t)readbytes;

  /* Fast path: if recv_buf is empty, it's just WANT_READ (no more data).
   * Avoids the expensive SSL_get_error() call which accesses OpenSSL's
   * thread-local error queue. This is the most common "failure" case. */
  if (conn->recv_buf_pos >= conn->recv_buf_len) {
    errno = EWOULDBLOCK;
    return -1;
  }

  /* Slow path: actual SSL error */
  int ssl_err = SSL_get_error(ssl, r);
  switch (ssl_err) {
  case SSL_ERROR_SSL:                   /* fall through */
  case SSL_ERROR_SYSCALL:               /* fall through */
  case SSL_ERROR_ZERO_RETURN: return 0; /* EOF */
  default: errno = EWOULDBLOCK; return -1;
  }
}

/**
 * Sends any unsent internal data. Returns:
 *   -1  on error / EWOULDBLOCK (pending data couldn't be fully flushed)
 *    0  when all internal buffers are empty (nothing left to send)
 *
 * With custom BIOs, all encrypted output is already in enc_buf. Flush simply
 * sends any pending enc_buf data to the socket.
 */
FIO_SFUNC int fio___openssl_flush(int fd, void *tls_ctx) {
  fio___openssl_connection_s *conn = (fio___openssl_connection_s *)tls_ctx;
  if (!conn)
    return 0;
  if (conn->enc_buf_sent >= conn->enc_buf_len) {
    conn->enc_buf_len = 0;
    conn->enc_buf_sent = 0;
    return 0;
  }
  fio___openssl_send_enc_buf(fd, conn);
  if (conn->enc_buf_sent < conn->enc_buf_len) {
    errno = EWOULDBLOCK;
    return -1;
  }
  return 0;
}

/**
 * Called to perform a non-blocking `write`, same as POSIX write(2).
 * Returns:
 *   N > 0  - number of plaintext bytes accepted/encrypted
 *   0      - nothing to write (len was 0)
 *   -1     - error or EWOULDBLOCK
 *
 * Custom BIO flow (zero intermediate copies):
 * 1. Flush any pending enc_buf data from a previous partial write
 * 2. Call SSL_write_ex() — OpenSSL encrypts plaintext and the custom wbio
 *    write callback appends ciphertext directly into enc_buf
 * 3. Send enc_buf to socket
 *
 * IMPORTANT: Once SSL_write_ex succeeds, OpenSSL's internal state (sequence
 * numbers, etc.) has advanced. We MUST return success even if the socket write
 * fails — the encrypted data is buffered in enc_buf and flush() sends later.
 */
FIO_SFUNC ssize_t fio___openssl_write(int fd,
                                      const void *buf,
                                      size_t len,
                                      void *tls_ctx) {
  if (!buf || !tls_ctx) {
    errno = EINVAL;
    return -1;
  }
  if (!len)
    return 0;
  fio___openssl_connection_s *conn = (fio___openssl_connection_s *)tls_ctx;

  /* Step 1: Flush any pending encrypted data from a previous partial write.
   * We cannot accept new plaintext until the old encrypted data is sent,
   * because enc_buf space is needed for the new encrypted output. */
  while (conn->enc_buf_sent < conn->enc_buf_len) {
    errno = 0;
    ssize_t written = fio_sock_write(fd,
                                     (char *)conn->enc_buf + conn->enc_buf_sent,
                                     conn->enc_buf_len - conn->enc_buf_sent);
    if (written <= 0) {
      errno = EWOULDBLOCK;
      return -1;
    }
    conn->enc_buf_sent += (size_t)written;
  }
  /* Pending data fully sent — reset buffer */
  conn->enc_buf_len = 0;
  conn->enc_buf_sent = 0;

  /* Step 2: Encrypt plaintext via SSL_write_ex.
   * OpenSSL writes encrypted output directly into enc_buf via custom wbio. */
  size_t ssl_written = 0;
  int r = SSL_write_ex(conn->ssl, buf, len, &ssl_written);

  if (r != 1 || ssl_written == 0) {
    int ssl_err = SSL_get_error(conn->ssl, r);
    switch (ssl_err) {
    case SSL_ERROR_SSL:                   /* fall through */
    case SSL_ERROR_SYSCALL:               /* fall through */
    case SSL_ERROR_ZERO_RETURN: return 0; /* connection error / EOF */
    default: errno = EWOULDBLOCK; return -1;
    }
  }

  /* Step 3: Send enc_buf (which now has encrypted data written by OpenSSL). */
  fio___openssl_send_enc_buf(fd, conn);

  /* CRITICAL: Return SUCCESS because SSL state has advanced.
   * Even if socket was full, data is buffered (in enc_buf) for flush(). */
  return (ssize_t)ssl_written;
}

/* *****************************************************************************
Per-Connection Builder
***************************************************************************** */

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

  /* Allocate per-connection wrapper */
  fio___openssl_connection_s *conn =
      (fio___openssl_connection_s *)FIO_MEM_REALLOC(NULL, 0, sizeof(*conn), 0);
  if (!conn) {
    FIO_LOG_ERROR("OpenSSL: connection allocation failed");
    SSL_free(ssl);
    return;
  }
  FIO_LEAK_COUNTER_ON_ALLOC(fio___openssl_connection_s);
  FIO_MEMSET(conn, 0, sizeof(*conn));
  conn->ssl = ssl;

  /* Create custom BIOs that read/write directly from/to our buffers */
  fio___openssl_init_bio_methods();
  BIO *rbio = BIO_new(fio___openssl_rbio_method);
  BIO *wbio = BIO_new(fio___openssl_wbio_method);
  if (!rbio || !wbio) {
    FIO_LOG_ERROR("OpenSSL: BIO_new(custom) failed");
    if (rbio)
      BIO_free(rbio);
    if (wbio)
      BIO_free(wbio);
    FIO_LEAK_COUNTER_ON_FREE(fio___openssl_connection_s);
    FIO_MEM_FREE(conn, sizeof(*conn));
    SSL_free(ssl);
    return;
  }
  BIO_set_data(rbio, conn); /* point to our connection struct */
  BIO_set_data(wbio, conn);
  BIO_set_init(rbio, 1); /* mark as initialized */
  BIO_set_init(wbio, 1);
  SSL_set_bio(ssl, rbio, wbio); /* SSL takes ownership of both BIOs */

  fio_io_tls_set(io, (void *)conn);

  FIO_LOG_DDEBUG2("(%d) allocated new TLS context (custom BIO) for %p.",
                  (int)fio_thread_getpid(),
                  (void *)io);

  SSL_set_ex_data(ssl, 0, (void *)io);

  /* Initiate handshake — OpenSSL writes handshake data into enc_buf via wbio */
  if (SSL_is_server(ssl))
    SSL_accept(ssl);
  else
    SSL_connect(ssl);

  /* Send any initial handshake output (ClientHello for client) directly.
   * For client connections, the ClientHello must be sent now because the
   * IO layer won't call on_ready/flush until the protocol layer queues outgoing
   * data — which won't happen until after the handshake completes. For server
   * connections, SSL_accept returns immediately (needs ClientHello first), so
   * enc_buf is typically empty here. */
  if (conn->enc_buf_len > conn->enc_buf_sent)
    fio___openssl_send_enc_buf(fio_io_fd(io), conn);
}

/* *****************************************************************************
Closing Connections
***************************************************************************** */

/** Called when the IO object finished sending all data before closure. */
FIO_SFUNC void fio___openssl_finish(int fd, void *tls_ctx) {
  fio___openssl_connection_s *conn = (fio___openssl_connection_s *)tls_ctx;
  if (!conn || !conn->ssl)
    return;

  /* SSL_shutdown writes close_notify directly into enc_buf via custom wbio */
  SSL_shutdown(conn->ssl);

  /* Best-effort send close_notify */
  if (conn->enc_buf_len > conn->enc_buf_sent)
    fio___openssl_send_enc_buf(fd, conn);
}

/* *****************************************************************************
Per-Connection Cleanup
***************************************************************************** */

/** Called after the IO object is closed, used to cleanup its `tls` object. */
FIO_SFUNC void fio___openssl_cleanup(void *tls_ctx) {
  fio___openssl_connection_s *conn = (fio___openssl_connection_s *)tls_ctx;
  if (!conn)
    return;
  if (conn->ssl) {
    SSL_shutdown(conn->ssl);
    SSL_free(conn->ssl); /* Also frees the attached BIOs */
  }
  FIO_LEAK_COUNTER_ON_FREE(fio___openssl_connection_s);
  FIO_MEM_FREE(conn, sizeof(*conn));
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
  fio___openssl_init_bio_methods();
  static fio_io_functions_s FIO___OPENSSL_IO_FUNCS;
  FIO___OPENSSL_IO_FUNCS = fio_openssl_io_functions();
  fio_io_tls_default_functions(&FIO___OPENSSL_IO_FUNCS);
  fio_state_callback_add(FIO_CALL_AT_EXIT,
                         fio___openssl_free_bio_methods,
                         NULL);
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
