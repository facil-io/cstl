/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_IO                 /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                  TLS 1.3 Implementation for IO Functions
                  (Drop-in replacement when OpenSSL unavailable)




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(H___FIO_IO___H) && defined(H___FIO_TLS13___H) &&                   \
    !defined(H___FIO_TLS13_IO___H) && !defined(FIO___RECURSIVE_INCLUDE)
#define H___FIO_TLS13_IO___H 1
/* *****************************************************************************
TLS 1.3 IO Function Getter
***************************************************************************** */

/** Returns the TLS 1.3 IO functions. */
SFUNC fio_io_functions_s fio_tls13_io_functions(void);

/* *****************************************************************************
TLS 1.3 IO Functions Implementation
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/* *****************************************************************************
TLS 1.3 Context Types
***************************************************************************** */

/** Context for all future connections (built from fio_io_tls_s) */
typedef struct {
  fio_io_tls_s *tls;
  uint8_t is_client;
  /* Certificate chain (DER-encoded) for server */
  uint8_t *cert_der;
  size_t cert_der_len;
  /* Private key for server (P-256: 32-byte scalar, Ed25519: 32-byte seed) */
  uint8_t private_key[32];
  /* Public key for P-256 signing (65 bytes: 0x04 || x || y) */
  uint8_t public_key[65];
  size_t private_key_len;
  uint16_t private_key_type;
  /* SNI hostname for client connections */
  char server_name[256];
} fio___tls13_context_s;

FIO_LEAK_COUNTER_DEF(fio___tls13_context_s)

/** Per-connection TLS state */
typedef struct {
  union {
    fio_tls13_client_s client;
    fio_tls13_server_s server;
  } state;
  uint8_t is_client;
  uint8_t handshake_complete;
  /* Buffered incoming data (partial records) */
  uint8_t *recv_buf;
  size_t recv_buf_len; /* Total data length in recv_buf */
  size_t recv_buf_pos; /* Current read position (lazy compaction) */
  size_t recv_buf_cap;
  /* Buffered decrypted data (ready to read) */
  uint8_t *app_buf;
  size_t app_buf_len;
  size_t app_buf_cap;
  size_t app_buf_pos; /* Read position in app_buf */
  /* Buffered outgoing handshake data */
  uint8_t *send_buf;
  size_t send_buf_len;
  size_t send_buf_cap;
  size_t send_buf_pos; /* Write position in send_buf */
  /* Pre-allocated encryption buffer (avoids stack allocation in write path) */
  uint8_t enc_buf[FIO_TLS13_MAX_CIPHERTEXT_LEN + FIO_TLS13_RECORD_HEADER_LEN +
                  FIO_TLS13_TAG_LEN + 16];
  /* Parent context (for certificate chain) */
  fio___tls13_context_s *ctx;
  /* Certificate chain storage (for server - pointers must outlive handshake) */
  const uint8_t *cert_ptr;
  size_t cert_len;
} fio___tls13_connection_s;

FIO_LEAK_COUNTER_DEF(fio___tls13_connection_s)

/* *****************************************************************************
TLS 1.3 Context Builder - Self-Signed Certificate Generation
***************************************************************************** */

#if defined(H___FIO_X509___H)
/** Global P-256 keypair for self-signed certificates */
static fio_x509_keypair_s fio___tls13_self_signed_keypair;
static uint8_t *fio___tls13_self_signed_cert = NULL;
static size_t fio___tls13_self_signed_cert_len = 0;

/** Cleanup callback to free the self-signed certificate at exit */
FIO_SFUNC void fio___tls13_clear_self_signed(void *ignr_) {
  if (fio___tls13_self_signed_cert) {
    FIO_MEM_FREE(fio___tls13_self_signed_cert,
                 fio___tls13_self_signed_cert_len);
    fio___tls13_self_signed_cert = NULL;
    fio___tls13_self_signed_cert_len = 0;
  }
  fio_x509_keypair_clear(&fio___tls13_self_signed_keypair);
  (void)ignr_;
}

/** Generate self-signed certificate for TLS 1.3 server */
FIO_SFUNC int fio___tls13_make_self_signed(const char *server_name) {
  static fio_lock_i lock = FIO_LOCK_INIT;
  fio_lock(&lock);

  if (fio___tls13_self_signed_cert) {
    fio_unlock(&lock);
    return 0; /* Already generated */
  }

  FIO_LOG_DEBUG2("generating P-256 self-signed certificate for TLS 1.3...");

  /* Generate P-256 keypair (universally supported by browsers/curl) */
  if (fio_x509_keypair_p256(&fio___tls13_self_signed_keypair) != 0) {
    FIO_LOG_ERROR("TLS 1.3: failed to generate P-256 keypair");
    fio_unlock(&lock);
    return -1;
  }

  /* Set up certificate options */
  if (!server_name || !server_name[0])
    server_name = "localhost";

  const char *san_dns[] = {server_name};
  fio_x509_cert_options_s opts = {
      .subject_cn = server_name,
      .subject_cn_len = FIO_STRLEN(server_name),
      .subject_org = "facil.io",
      .subject_org_len = 8,
      .san_dns = san_dns,
      .san_dns_count = 1,
      .is_ca = 0,
  };

  /* Calculate certificate size */
  size_t cert_size = fio_x509_self_signed_cert(NULL,
                                               0,
                                               &fio___tls13_self_signed_keypair,
                                               &opts);
  if (cert_size == 0) {
    FIO_LOG_ERROR("TLS 1.3: failed to calculate certificate size");
    fio_x509_keypair_clear(&fio___tls13_self_signed_keypair);
    fio_unlock(&lock);
    return -1;
  }

  /* Allocate and generate certificate */
  fio___tls13_self_signed_cert =
      (uint8_t *)FIO_MEM_REALLOC(NULL, 0, cert_size, 0);
  if (!fio___tls13_self_signed_cert) {
    FIO_LOG_ERROR("TLS 1.3: failed to allocate certificate buffer");
    fio_x509_keypair_clear(&fio___tls13_self_signed_keypair);
    fio_unlock(&lock);
    return -1;
  }

  fio___tls13_self_signed_cert_len =
      fio_x509_self_signed_cert(fio___tls13_self_signed_cert,
                                cert_size,
                                &fio___tls13_self_signed_keypair,
                                &opts);

  if (fio___tls13_self_signed_cert_len == 0) {
    FIO_LOG_ERROR("TLS 1.3: failed to generate self-signed certificate");
    FIO_MEM_FREE(fio___tls13_self_signed_cert, cert_size);
    fio___tls13_self_signed_cert = NULL;
    fio_x509_keypair_clear(&fio___tls13_self_signed_keypair);
    fio_unlock(&lock);
    return -1;
  }

  /* Register cleanup callback */
  fio_state_callback_add(FIO_CALL_AT_EXIT, fio___tls13_clear_self_signed, NULL);

  FIO_LOG_DEBUG2("P-256 self-signed certificate generated successfully "
                 "(%zu bytes)",
                 fio___tls13_self_signed_cert_len);
  fio_unlock(&lock);
  return 0;
}
#endif /* H___FIO_X509___H */

/* *****************************************************************************
TLS 1.3 Context Builder
***************************************************************************** */

/** Callback for iterating certificates in fio_io_tls_s */
FIO_SFUNC int fio___tls13_each_cert(struct fio_io_tls_each_s *e,
                                    const char *server_name,
                                    const char *public_cert_file,
                                    const char *private_key_file,
                                    const char *pk_password) {
  fio___tls13_context_s *ctx = (fio___tls13_context_s *)e->udata;

#if defined(H___FIO_PEM___H) && defined(H___FIO_X509___H)
  /* Try to load certificate and key from PEM files */
  if (public_cert_file && private_key_file) {
    FIO_LOG_DEBUG2("TLS 1.3: loading certificate from %s", public_cert_file);
    FIO_LOG_DEBUG2("TLS 1.3: loading private key from %s", private_key_file);

    /* Read certificate file */
    char *cert_pem = fio_bstr_readfile(NULL, public_cert_file, 0, 0);
    if (!cert_pem) {
      FIO_LOG_ERROR("TLS 1.3: failed to read certificate file: %s",
                    public_cert_file);
      goto use_self_signed;
    }

    /* Read private key file */
    char *key_pem = fio_bstr_readfile(NULL, private_key_file, 0, 0);
    if (!key_pem) {
      FIO_LOG_ERROR("TLS 1.3: failed to read private key file: %s",
                    private_key_file);
      fio_bstr_free(cert_pem);
      goto use_self_signed;
    }

    /* Allocate buffer for DER-encoded certificate */
    size_t cert_pem_len = fio_bstr_len(cert_pem);
    size_t der_buf_len = cert_pem_len; /* Conservative estimate */
    ctx->cert_der = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, der_buf_len, 0);
    if (!ctx->cert_der) {
      FIO_LOG_ERROR("TLS 1.3: failed to allocate certificate buffer");
      fio_bstr_free(cert_pem);
      fio_bstr_free(key_pem);
      return -1;
    }

    /* Extract DER from PEM */
    ctx->cert_der_len = fio_pem_get_certificate_der(ctx->cert_der,
                                                    der_buf_len,
                                                    cert_pem,
                                                    cert_pem_len);
    fio_bstr_free(cert_pem);

    if (ctx->cert_der_len == 0) {
      FIO_LOG_ERROR("TLS 1.3: failed to parse certificate PEM");
      FIO_MEM_FREE(ctx->cert_der, der_buf_len);
      ctx->cert_der = NULL;
      fio_bstr_free(key_pem);
      goto use_self_signed;
    }

    /* Parse private key */
    fio_pem_private_key_s pkey;
    size_t key_pem_len = fio_bstr_len(key_pem);
    if (fio_pem_parse_private_key(&pkey, key_pem, key_pem_len) != 0) {
      FIO_LOG_ERROR("TLS 1.3: failed to parse private key PEM");
      FIO_MEM_FREE(ctx->cert_der, der_buf_len);
      ctx->cert_der = NULL;
      ctx->cert_der_len = 0;
      fio_bstr_free(key_pem);
      goto use_self_signed;
    }
    fio_bstr_free(key_pem);

    /* Copy private key based on type */
    switch (pkey.type) {
    case FIO_PEM_KEY_ECDSA_P256:
      FIO_MEMCPY(ctx->private_key, pkey.ecdsa_p256.private_key, 32);
      if (pkey.ecdsa_p256.has_public_key) {
        FIO_MEMCPY(ctx->public_key, pkey.ecdsa_p256.public_key, 65);
      } else {
        /* Derive public key from private key */
#if defined(H___FIO_P256___H)
        uint8_t tmp_secret[32];
        FIO_MEMCPY(tmp_secret, pkey.ecdsa_p256.private_key, 32);
        fio_p256_keypair(tmp_secret, ctx->public_key);
        fio_secure_zero(tmp_secret, 32);
#else
        FIO_LOG_ERROR("TLS 1.3: P-256 module required for key derivation");
        fio_pem_private_key_clear(&pkey);
        FIO_MEM_FREE(ctx->cert_der, der_buf_len);
        ctx->cert_der = NULL;
        ctx->cert_der_len = 0;
        goto use_self_signed;
#endif
      }
      ctx->private_key_len = 32;
      ctx->private_key_type = FIO_TLS13_SIG_ECDSA_SECP256R1_SHA256;
      FIO_LOG_DEBUG2("TLS 1.3: loaded P-256 private key from PEM");
      break;

    case FIO_PEM_KEY_ED25519:
      FIO_MEMCPY(ctx->private_key, pkey.ed25519.private_key, 32);
      ctx->private_key_len = 32;
      ctx->private_key_type = FIO_TLS13_SIG_ED25519;
      FIO_LOG_DEBUG2("TLS 1.3: loaded Ed25519 private key from PEM");
      break;

    case FIO_PEM_KEY_RSA:
      /* RSA signing not yet supported in TLS 1.3 implementation */
      FIO_LOG_WARNING(
          "TLS 1.3: RSA private keys not yet supported for signing");
      fio_pem_private_key_clear(&pkey);
      FIO_MEM_FREE(ctx->cert_der, der_buf_len);
      ctx->cert_der = NULL;
      ctx->cert_der_len = 0;
      goto use_self_signed;

    default:
      FIO_LOG_ERROR("TLS 1.3: unsupported private key type");
      fio_pem_private_key_clear(&pkey);
      FIO_MEM_FREE(ctx->cert_der, der_buf_len);
      ctx->cert_der = NULL;
      ctx->cert_der_len = 0;
      goto use_self_signed;
    }

    fio_pem_private_key_clear(&pkey);
    FIO_LOG_DEBUG2("TLS 1.3: certificate loaded successfully (%zu bytes)",
                   ctx->cert_der_len);
    (void)pk_password;
    (void)server_name;
    return 0;
  }

use_self_signed:
#endif /* H___FIO_PEM___H && H___FIO_X509___H */

#if defined(H___FIO_X509___H)
  /* Generate self-signed certificate if no PEM files provided or loading failed
   */
  if (public_cert_file && private_key_file) {
    FIO_LOG_WARNING(
        "TLS 1.3: PEM loading failed, using self-signed certificate");
  }

  if (fio___tls13_make_self_signed(server_name) != 0) {
    FIO_LOG_ERROR("TLS 1.3: failed to create self-signed certificate");
    return -1;
  }

  /* Copy certificate to context */
  if (fio___tls13_self_signed_cert && fio___tls13_self_signed_cert_len > 0) {
    ctx->cert_der = (uint8_t *)
        FIO_MEM_REALLOC(NULL, 0, fio___tls13_self_signed_cert_len, 0);
    if (!ctx->cert_der) {
      FIO_LOG_ERROR("TLS 1.3: failed to allocate certificate buffer");
      return -1;
    }
    FIO_MEMCPY(ctx->cert_der,
               fio___tls13_self_signed_cert,
               fio___tls13_self_signed_cert_len);
    ctx->cert_der_len = fio___tls13_self_signed_cert_len;

    /* Copy private key (P-256 scalar) */
    FIO_MEMCPY(ctx->private_key,
               fio___tls13_self_signed_keypair.secret_key,
               32);
    /* Copy public key (P-256 uncompressed point for signing) */
    FIO_MEMCPY(ctx->public_key, fio___tls13_self_signed_keypair.public_key, 65);
    ctx->private_key_len = 32;
    ctx->private_key_type = FIO_TLS13_SIG_ECDSA_SECP256R1_SHA256;
  }
#else
  FIO_LOG_ERROR(
      "TLS 1.3: X509 module not available for certificate generation");
  return -1;
#endif

  (void)pk_password;
  return 0;
}

/** Helper that converts a `fio_io_tls_s` into the implementation's context. */
FIO_SFUNC void *fio___tls13_build_context(fio_io_tls_s *tls,
                                          uint8_t is_client) {
  fio___tls13_context_s *ctx =
      (fio___tls13_context_s *)FIO_MEM_REALLOC(NULL, 0, sizeof(*ctx), 0);
  if (!ctx) {
    FIO_LOG_ERROR("TLS 1.3: memory allocation failed for context");
    return NULL;
  }
  FIO_LEAK_COUNTER_ON_ALLOC(fio___tls13_context_s);
  FIO_MEMSET(ctx, 0, sizeof(*ctx));
  ctx->tls = fio_io_tls_dup(tls);
  ctx->is_client = is_client;

  /* For client, extract server name from TLS object for SNI */
  if (is_client) {
    FIO_IMAP_EACH(fio___io_tls_cert_map, &tls->cert, i) {
      fio_buf_info_s nm = fio_keystr_buf(&tls->cert.ary[i].nm);
      if (nm.buf && nm.len > 0 && nm.len < sizeof(ctx->server_name)) {
        FIO_MEMCPY(ctx->server_name, nm.buf, nm.len);
        ctx->server_name[nm.len] = '\0';
        FIO_LOG_DEBUG2("TLS 1.3: extracted SNI hostname: %s", ctx->server_name);
        break;
      }
    }
  } else { /* For server, load certificates */
    /* Check if certificates are configured */
    if (!fio_io_tls_cert_count(tls)) {
      /* No certificates configured - use self-signed */
#if defined(H___FIO_X509___H)
      if (fio___tls13_make_self_signed("localhost") != 0) {
        FIO_LOG_ERROR("TLS 1.3: failed to create self-signed certificate");
        goto error;
      }
      ctx->cert_der = (uint8_t *)
          FIO_MEM_REALLOC(NULL, 0, fio___tls13_self_signed_cert_len, 0);
      if (!ctx->cert_der) {
        FIO_LOG_ERROR("TLS 1.3: failed to allocate certificate buffer");
        goto error;
      }
      FIO_MEMCPY(ctx->cert_der,
                 fio___tls13_self_signed_cert,
                 fio___tls13_self_signed_cert_len);
      ctx->cert_der_len = fio___tls13_self_signed_cert_len;
      /* Copy private key (P-256 scalar) */
      FIO_MEMCPY(ctx->private_key,
                 fio___tls13_self_signed_keypair.secret_key,
                 32);
      /* Copy public key (P-256 uncompressed point for signing) */
      FIO_MEMCPY(ctx->public_key,
                 fio___tls13_self_signed_keypair.public_key,
                 65);
      ctx->private_key_len = 32;
      ctx->private_key_type = FIO_TLS13_SIG_ECDSA_SECP256R1_SHA256;
#else
      FIO_LOG_ERROR("TLS 1.3: X509 module required for server certificates");
      goto error;
#endif
    } else {
      /* Process configured certificates */
      if (fio_io_tls_each(tls,
                          .udata = ctx,
                          .each_cert = fio___tls13_each_cert)) {
        FIO_LOG_ERROR("TLS 1.3: failed to configure certificates");
        goto error;
      }
    }
  }

  return ctx;

error:
  if (ctx) {
    if (ctx->cert_der)
      FIO_MEM_FREE(ctx->cert_der, ctx->cert_der_len);
    if (ctx->tls)
      fio_io_tls_free(ctx->tls);
    FIO_LEAK_COUNTER_ON_FREE(fio___tls13_context_s);
    FIO_MEM_FREE(ctx, sizeof(*ctx));
  }
  return NULL;
}

/* *****************************************************************************
TLS 1.3 Context Cleanup
***************************************************************************** */

FIO_SFUNC void fio___tls13_free_context_task(void *tls_ctx, void *ignr_) {
  fio___tls13_context_s *ctx = (fio___tls13_context_s *)tls_ctx;
  if (!ctx)
    return;
  FIO_LEAK_COUNTER_ON_FREE(fio___tls13_context_s);
  if (ctx->cert_der)
    FIO_MEM_FREE(ctx->cert_der, ctx->cert_der_len);
  fio_secure_zero(ctx->private_key, sizeof(ctx->private_key));
  if (ctx->tls)
    fio_io_tls_free(ctx->tls);
  FIO_MEM_FREE(ctx, sizeof(*ctx));
  (void)ignr_;
}

/** Helper to free the context built by build_context. */
FIO_SFUNC void fio___tls13_free_context(void *tls_ctx) {
  fio_io_defer(fio___tls13_free_context_task, tls_ctx, NULL);
}

/* *****************************************************************************
TLS 1.3 Per-Connection Management
***************************************************************************** */

/** Allocate connection state */
FIO_SFUNC fio___tls13_connection_s *fio___tls13_connection_new(
    fio___tls13_context_s *ctx) {
  fio___tls13_connection_s *conn =
      (fio___tls13_connection_s *)FIO_MEM_REALLOC(NULL, 0, sizeof(*conn), 0);
  if (!conn)
    return NULL;
  FIO_LEAK_COUNTER_ON_ALLOC(fio___tls13_connection_s);
  FIO_MEMSET(conn, 0, sizeof(*conn));
  conn->is_client = ctx->is_client;
  conn->ctx = ctx;

  /* Allocate buffers */
  conn->recv_buf_cap = FIO_TLS13_MAX_CIPHERTEXT_LEN + 256;
  conn->recv_buf = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, conn->recv_buf_cap, 0);
  if (!conn->recv_buf)
    goto error;

  conn->app_buf_cap = FIO_TLS13_MAX_PLAINTEXT_LEN;
  conn->app_buf = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, conn->app_buf_cap, 0);
  if (!conn->app_buf)
    goto error;

  conn->send_buf_cap = 8192;
  conn->send_buf = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, conn->send_buf_cap, 0);
  if (!conn->send_buf)
    goto error;

  return conn;

error:
  if (conn->recv_buf)
    FIO_MEM_FREE(conn->recv_buf, conn->recv_buf_cap);
  if (conn->app_buf)
    FIO_MEM_FREE(conn->app_buf, conn->app_buf_cap);
  if (conn->send_buf)
    FIO_MEM_FREE(conn->send_buf, conn->send_buf_cap);
  FIO_LEAK_COUNTER_ON_FREE(fio___tls13_connection_s);
  FIO_MEM_FREE(conn, sizeof(*conn));
  return NULL;
}

/** Free connection state */
FIO_SFUNC void fio___tls13_connection_free(fio___tls13_connection_s *conn) {
  if (!conn)
    return;

  /* Destroy TLS state */
  if (conn->is_client)
    fio_tls13_client_destroy(&conn->state.client);
  else
    fio_tls13_server_destroy(&conn->state.server);

  /* Free buffers */
  if (conn->recv_buf)
    FIO_MEM_FREE(conn->recv_buf, conn->recv_buf_cap);
  if (conn->app_buf)
    FIO_MEM_FREE(conn->app_buf, conn->app_buf_cap);
  if (conn->send_buf)
    FIO_MEM_FREE(conn->send_buf, conn->send_buf_cap);

  FIO_LEAK_COUNTER_ON_FREE(fio___tls13_connection_s);
  FIO_MEM_FREE(conn, sizeof(*conn));
}

/* *****************************************************************************
TLS 1.3 IO Functions - Start
***************************************************************************** */

/** Called when a new IO is first attached to a valid protocol. */
FIO_SFUNC void fio___tls13_start(fio_io_s *io) {
  fio___tls13_context_s *ctx = (fio___tls13_context_s *)fio_io_tls(io);
  if (!ctx) {
    FIO_LOG_ERROR("TLS 1.3: Context missing for connection!");
    return;
  }

  /* Allocate connection state */
  fio___tls13_connection_s *conn = fio___tls13_connection_new(ctx);
  if (!conn) {
    FIO_LOG_ERROR("TLS 1.3: failed to allocate connection state");
    return;
  }

  /* Store connection state in IO */
  fio_io_tls_set(io, (void *)conn);

  FIO_LOG_DDEBUG2("(%d) TLS 1.3: allocated new connection for %p (%s)",
                  (int)fio_thread_getpid(),
                  (void *)io,
                  conn->is_client ? "client" : "server");

  if (conn->is_client) {
    /* Initialize client and start handshake with SNI */
    const char *sni = ctx->server_name[0] ? ctx->server_name : NULL;
    fio_tls13_client_init(&conn->state.client, sni);
    fio_tls13_client_skip_verification(&conn->state.client,
                                       1); /* TODO: proper verification */

    /* Generate ClientHello */
    int ch_len = fio_tls13_client_start(&conn->state.client,
                                        conn->send_buf,
                                        conn->send_buf_cap);
    if (ch_len < 0) {
      FIO_LOG_ERROR("TLS 1.3: failed to generate ClientHello");
      return;
    }
    conn->send_buf_len = (size_t)ch_len;
    conn->send_buf_pos = 0;
  } else {
    /* Initialize server */
    fio_tls13_server_init(&conn->state.server);

    /* Set certificate chain - use pointers stored in connection struct */
    if (ctx->cert_der && ctx->cert_der_len > 0) {
      /* Store cert pointer and length in connection for lifetime management */
      conn->cert_ptr = ctx->cert_der;
      conn->cert_len = ctx->cert_der_len;
      fio_tls13_server_set_cert_chain(&conn->state.server,
                                      &conn->cert_ptr,
                                      &conn->cert_len,
                                      1);
    }

    /* Set private key */
    if (ctx->private_key_len > 0) {
      fio_tls13_server_set_private_key(&conn->state.server,
                                       ctx->private_key,
                                       ctx->private_key_len,
                                       ctx->private_key_type);
      /* Copy public key for P-256 signing */
      if (ctx->private_key_type == FIO_TLS13_SIG_ECDSA_SECP256R1_SHA256) {
        FIO_MEMCPY(conn->state.server.public_key, ctx->public_key, 65);
      }
    }
  }
}

/* *****************************************************************************
TLS 1.3 IO Functions - Read
***************************************************************************** */

/** Called to perform a non-blocking `read`, same as the system call. */
FIO_SFUNC ssize_t fio___tls13_read(int fd,
                                   void *buf,
                                   size_t len,
                                   void *tls_ctx) {
  fio___tls13_connection_s *conn = (fio___tls13_connection_s *)tls_ctx;
  if (!conn || !buf)
    return -1;

  /* If we have buffered application data, return it first */
  if (conn->app_buf_len > conn->app_buf_pos) {
    size_t available = conn->app_buf_len - conn->app_buf_pos;
    size_t to_copy = (len < available) ? len : available;
    FIO_MEMCPY(buf, conn->app_buf + conn->app_buf_pos, to_copy);
    conn->app_buf_pos += to_copy;

    /* Reset buffer if fully consumed */
    if (conn->app_buf_pos >= conn->app_buf_len) {
      conn->app_buf_len = 0;
      conn->app_buf_pos = 0;
    }
    return (ssize_t)to_copy;
  }

  /* Calculate available space in recv_buf and compact if needed */
  size_t recv_data_len = conn->recv_buf_len - conn->recv_buf_pos;
  size_t recv_space = conn->recv_buf_cap - conn->recv_buf_len;

  /* Lazy compaction: only compact when buffer is >50% consumed AND
   * we don't have enough space for a new record */
  if (conn->recv_buf_pos > (conn->recv_buf_cap >> 1) &&
      recv_space < FIO_TLS13_MAX_CIPHERTEXT_LEN) {
    if (recv_data_len > 0) {
      FIO_MEMMOVE(conn->recv_buf,
                  conn->recv_buf + conn->recv_buf_pos,
                  recv_data_len);
    }
    conn->recv_buf_len = recv_data_len;
    conn->recv_buf_pos = 0;
    recv_space = conn->recv_buf_cap - conn->recv_buf_len;
  }

  /* Read raw data from socket */
  errno = 0;
  ssize_t raw_read =
      fio_sock_read(fd, conn->recv_buf + conn->recv_buf_len, recv_space);
  if (raw_read <= 0) {
    if (raw_read == 0)
      return 0; /* EOF */
    if (errno == EWOULDBLOCK || errno == EAGAIN)
      return -1;
    return raw_read;
  }
  conn->recv_buf_len += (size_t)raw_read;
  recv_data_len = conn->recv_buf_len - conn->recv_buf_pos;

  /* Process received data */
  uint8_t *recv_ptr = conn->recv_buf + conn->recv_buf_pos;
  while (recv_data_len >= FIO_TLS13_RECORD_HEADER_LEN) {
    /* Check if we have a complete record */
    uint16_t record_len = ((uint16_t)recv_ptr[3] << 8) | recv_ptr[4];
    size_t total_record_len = FIO_TLS13_RECORD_HEADER_LEN + record_len;

    if (recv_data_len < total_record_len)
      break; /* Need more data */

    if (!conn->handshake_complete) {
      /* Process handshake */
      uint8_t out_buf[8192];
      size_t out_len = 0;
      int consumed;

      if (conn->is_client) {
        consumed = fio_tls13_client_process(&conn->state.client,
                                            recv_ptr,
                                            recv_data_len,
                                            out_buf,
                                            sizeof(out_buf),
                                            &out_len);
      } else {
        consumed = fio_tls13_server_process(&conn->state.server,
                                            recv_ptr,
                                            recv_data_len,
                                            out_buf,
                                            sizeof(out_buf),
                                            &out_len);
      }

      if (consumed < 0) {
        FIO_LOG_DEBUG2("TLS 1.3: handshake processing error");
        errno = ECONNRESET;
        return -1;
      }

      /* Buffer any handshake response */
      if (out_len > 0) {
        if (conn->send_buf_len + out_len <= conn->send_buf_cap) {
          FIO_MEMCPY(conn->send_buf + conn->send_buf_len, out_buf, out_len);
          conn->send_buf_len += out_len;
        }
        /* Immediately try to send handshake response */
        while (conn->send_buf_pos < conn->send_buf_len) {
          errno = 0;
          ssize_t hs_written =
              fio_sock_write(fd,
                             conn->send_buf + conn->send_buf_pos,
                             conn->send_buf_len - conn->send_buf_pos);
          if (hs_written <= 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
              break; /* Will retry later */
            break;
          }
          conn->send_buf_pos += (size_t)hs_written;
        }
        if (conn->send_buf_pos >= conn->send_buf_len) {
          conn->send_buf_len = 0;
          conn->send_buf_pos = 0;
        }
      }

      /* Advance read position (lazy compaction) */
      if (consumed > 0 && (size_t)consumed <= recv_data_len) {
        conn->recv_buf_pos += (size_t)consumed;
        recv_ptr += consumed;
        recv_data_len -= (size_t)consumed;
      }

      /* Check if handshake is complete */
      if (conn->is_client) {
        if (fio_tls13_client_is_connected(&conn->state.client)) {
          conn->handshake_complete = 1;
          FIO_LOG_DEBUG2("TLS 1.3: client handshake complete");
        } else if (fio_tls13_client_is_error(&conn->state.client)) {
          FIO_LOG_DEBUG2("TLS 1.3: client handshake error");
          errno = ECONNRESET;
          return -1;
        }
      } else {
        if (fio_tls13_server_is_connected(&conn->state.server)) {
          conn->handshake_complete = 1;
          FIO_LOG_DEBUG2("TLS 1.3: server handshake complete");
        } else if (fio_tls13_server_is_error(&conn->state.server)) {
          FIO_LOG_DEBUG2("TLS 1.3: server handshake error");
          errno = ECONNRESET;
          return -1;
        }
      }
    } else {
      /* Decrypt application data */
      int dec_len;

      /* OPTIMIZATION: If user buffer is large enough and app_buf is empty,
       * decrypt directly into user buffer to avoid double copy */
      size_t expected_plaintext_len = record_len > (FIO_TLS13_TAG_LEN + 1)
                                          ? record_len - FIO_TLS13_TAG_LEN - 1
                                          : 0;
      uint8_t *decrypt_target;
      size_t decrypt_capacity;

      if (len >= expected_plaintext_len && conn->app_buf_len == 0 &&
          conn->app_buf_pos == 0) {
        /* Decrypt directly to user buffer */
        decrypt_target = (uint8_t *)buf;
        decrypt_capacity = len;
      } else {
        /* Use app_buf as intermediate buffer */
        decrypt_target = conn->app_buf + conn->app_buf_len;
        decrypt_capacity = conn->app_buf_cap - conn->app_buf_len;
      }

      if (conn->is_client) {
        dec_len = fio_tls13_client_decrypt(&conn->state.client,
                                           decrypt_target,
                                           decrypt_capacity,
                                           recv_ptr,
                                           total_record_len);
      } else {
        dec_len = fio_tls13_server_decrypt(&conn->state.server,
                                           decrypt_target,
                                           decrypt_capacity,
                                           recv_ptr,
                                           total_record_len);
      }

      if (dec_len < 0) {
        FIO_LOG_DEBUG2("TLS 1.3: decryption error");
        errno = ECONNRESET;
        return -1;
      }

      /* Advance read position (lazy compaction) */
      conn->recv_buf_pos += total_record_len;
      recv_ptr += total_record_len;
      recv_data_len -= total_record_len;

      /* If we decrypted directly to user buffer, return immediately */
      if (decrypt_target == (uint8_t *)buf && dec_len > 0) {
        return (ssize_t)dec_len;
      }

      if (dec_len > 0)
        conn->app_buf_len += (size_t)dec_len;
    }
  }

  /* Return buffered application data */
  if (conn->app_buf_len > conn->app_buf_pos) {
    size_t available = conn->app_buf_len - conn->app_buf_pos;
    size_t to_copy = (len < available) ? len : available;
    FIO_MEMCPY(buf, conn->app_buf + conn->app_buf_pos, to_copy);
    conn->app_buf_pos += to_copy;

    if (conn->app_buf_pos >= conn->app_buf_len) {
      conn->app_buf_len = 0;
      conn->app_buf_pos = 0;
    }
    return (ssize_t)to_copy;
  }

  /* No data available yet */
  errno = EWOULDBLOCK;
  return -1;
}

/* *****************************************************************************
TLS 1.3 IO Functions - Write
***************************************************************************** */

/** Called to perform a non-blocking `write`, same as the system call. */
FIO_SFUNC ssize_t fio___tls13_write(int fd,
                                    const void *buf,
                                    size_t len,
                                    void *tls_ctx) {
  fio___tls13_connection_s *conn = (fio___tls13_connection_s *)tls_ctx;
  if (!conn || !buf || len == 0)
    return -1;

  /* If handshake not complete, can't send application data */
  if (!conn->handshake_complete) {
    errno = EWOULDBLOCK;
    return -1;
  }

  /* Flush any pending handshake data (e.g., client Finished) before sending
   * application data. The server must receive the client Finished before it
   * can process application data encrypted with application keys. */
  while (conn->send_buf_pos < conn->send_buf_len) {
    errno = 0;
    ssize_t hs_written =
        fio_sock_write(fd,
                       conn->send_buf + conn->send_buf_pos,
                       conn->send_buf_len - conn->send_buf_pos);
    if (hs_written <= 0) {
      /* Can't send handshake data yet, so can't send app data either */
      errno = EWOULDBLOCK;
      return -1;
    }
    conn->send_buf_pos += (size_t)hs_written;
  }
  /* Reset send buffer after handshake data is fully sent */
  if (conn->send_buf_pos >= conn->send_buf_len) {
    conn->send_buf_len = 0;
    conn->send_buf_pos = 0;
  }

  /* Limit to max plaintext size */
  if (len > FIO_TLS13_MAX_PLAINTEXT_LEN)
    len = FIO_TLS13_MAX_PLAINTEXT_LEN;

  /* Encrypt data using pre-allocated buffer (avoids stack allocation) */
  int enc_len;
  if (conn->is_client) {
    enc_len = fio_tls13_client_encrypt(&conn->state.client,
                                       conn->enc_buf,
                                       sizeof(conn->enc_buf),
                                       (const uint8_t *)buf,
                                       len);
  } else {
    enc_len = fio_tls13_server_encrypt(&conn->state.server,
                                       conn->enc_buf,
                                       sizeof(conn->enc_buf),
                                       (const uint8_t *)buf,
                                       len);
  }

  if (enc_len < 0) {
    FIO_LOG_DEBUG2("TLS 1.3: encryption error");
    errno = ECONNRESET;
    return -1;
  }

  /* Write encrypted data to socket */
  errno = 0;
  ssize_t written = fio_sock_write(fd, conn->enc_buf, (size_t)enc_len);
  if (written <= 0) {
    if (errno == EWOULDBLOCK || errno == EAGAIN)
      return -1;
    return written;
  }

  /* If we wrote all encrypted data, report original plaintext length */
  if (written == enc_len)
    return (ssize_t)len;

  /* Partial write - this is tricky with TLS, report error */
  /* TODO: Buffer partial writes properly */
  errno = EWOULDBLOCK;
  return -1;
}

/* *****************************************************************************
TLS 1.3 IO Functions - Flush
***************************************************************************** */

/** Sends any unsent internal data. Returns 0 only if all data was sent. */
FIO_SFUNC int fio___tls13_flush(int fd, void *tls_ctx) {
  fio___tls13_connection_s *conn = (fio___tls13_connection_s *)tls_ctx;
  if (!conn)
    return 0;

  /* Send any buffered handshake data */
  while (conn->send_buf_pos < conn->send_buf_len) {
    errno = 0;
    ssize_t written = fio_sock_write(fd,
                                     conn->send_buf + conn->send_buf_pos,
                                     conn->send_buf_len - conn->send_buf_pos);
    if (written <= 0) {
      if (errno == EWOULDBLOCK || errno == EAGAIN)
        return -1; /* Would block, try again later */
      return -1;   /* Error */
    }
    conn->send_buf_pos += (size_t)written;
  }

  /* Reset send buffer if fully sent */
  if (conn->send_buf_pos >= conn->send_buf_len) {
    conn->send_buf_len = 0;
    conn->send_buf_pos = 0;
  }

  return (conn->send_buf_len > conn->send_buf_pos) ? -1 : 0;
}

/* *****************************************************************************
TLS 1.3 IO Functions - Finish
***************************************************************************** */

/** Called when the IO object finished sending all data before closure. */
FIO_SFUNC void fio___tls13_finish(int fd, void *tls_ctx) {
  fio___tls13_connection_s *conn = (fio___tls13_connection_s *)tls_ctx;
  if (!conn)
    return;

  /* Send close_notify alert */
  if (conn->handshake_complete) {
    uint8_t alert[32];
    /* Build close_notify alert: level=warning(1), description=close_notify(0)
     */
    uint8_t alert_data[2] = {1, 0};
    int enc_len;

    if (conn->is_client) {
      enc_len = fio_tls13_record_encrypt(alert,
                                         sizeof(alert),
                                         alert_data,
                                         2,
                                         FIO_TLS13_CONTENT_ALERT,
                                         &conn->state.client.client_app_keys);
    } else {
      enc_len = fio_tls13_record_encrypt(alert,
                                         sizeof(alert),
                                         alert_data,
                                         2,
                                         FIO_TLS13_CONTENT_ALERT,
                                         &conn->state.server.server_app_keys);
    }

    if (enc_len > 0) {
      /* Best effort send, ignore errors */
      (void)fio_sock_write(fd, alert, (size_t)enc_len);
    }
  }
  (void)fd;
}

/* *****************************************************************************
TLS 1.3 IO Functions - Cleanup
***************************************************************************** */

/** Called after the IO object is closed, used to cleanup its `tls` object. */
FIO_SFUNC void fio___tls13_cleanup(void *tls_ctx) {
  fio___tls13_connection_s *conn = (fio___tls13_connection_s *)tls_ctx;
  fio___tls13_connection_free(conn);
}

/* *****************************************************************************
TLS 1.3 IO Functions Structure
***************************************************************************** */

/** Returns the TLS 1.3 IO functions (implementation) */
SFUNC fio_io_functions_s fio_tls13_io_functions(void) {
  return (fio_io_functions_s){
      .build_context = fio___tls13_build_context,
      .free_context = fio___tls13_free_context,
      .start = fio___tls13_start,
      .read = fio___tls13_read,
      .write = fio___tls13_write,
      .flush = fio___tls13_flush,
      .cleanup = fio___tls13_cleanup,
      .finish = fio___tls13_finish,
  };
}

/* *****************************************************************************
TLS 1.3 Default Setup (when OpenSSL is not available)
***************************************************************************** */

#if !defined(HAVE_OPENSSL) && !defined(H___FIO_OPENSSL___H)
/** Setup TLS 1.3 as TLS IO default when OpenSSL is not available */
FIO_CONSTRUCTOR(fio___tls13_setup_default) {
  static fio_io_functions_s FIO___TLS13_IO_FUNCS;
  FIO___TLS13_IO_FUNCS = fio_tls13_io_functions();
  fio_io_tls_default_functions(&FIO___TLS13_IO_FUNCS);
  FIO_LOG_DEBUG2("TLS 1.3 registered as default TLS implementation");
}
#endif /* !HAVE_OPENSSL && !H___FIO_OPENSSL___H */

/* *****************************************************************************
TLS 1.3 IO Functions Cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#endif /* H___FIO_IO___H && H___FIO_TLS13___H */
