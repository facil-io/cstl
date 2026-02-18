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
  /* ALPN protocols (comma-separated) for server */
  char alpn_protocols[256];
  size_t alpn_protocols_len;
#if defined(H___FIO_X509___H) && defined(H___FIO_PEM___H)
  /* Trust store for client certificate verification (NULL = skip verify) */
  fio_x509_trust_store_s trust_store;
  /* Owned DER buffers backing trust_store.roots (freed with context) */
  uint8_t **trust_der_bufs;
  size_t *trust_der_lens;
  size_t trust_der_count;
#endif /* H___FIO_X509___H && H___FIO_PEM___H */
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
  /* Buffered incoming data (partial records) - stored in buf[0..cap) */
  size_t recv_buf_len; /* Total data length in recv_buf */
  size_t recv_buf_pos; /* Current read position (lazy compaction) */
  /* Buffered decrypted data (ready to read) - stored in buf[cap..2*cap) */
  size_t app_buf_len;
  size_t app_buf_pos; /* Read position in app_buf */
  /* Buffered outgoing handshake data - stored in buf[2*cap..3*cap) */
  size_t send_buf_len;
  size_t send_buf_pos; /* Write position in send_buf */
  /* Pre-allocated encryption buffer for multi-record batching.
   * Sized for 4 max TLS records to match FIO_IO_BUFFER_PER_WRITE (64KB).
   * The IO layer offers up to 64KB per write; each TLS record holds up to
   * 16384 bytes of plaintext with 22 bytes overhead, so 4 records cover it. */
#define FIO___TLS13_ENC_RECORD_SIZE                                            \
  (FIO_TLS13_MAX_CIPHERTEXT_LEN + FIO_TLS13_RECORD_HEADER_LEN +                \
   FIO_TLS13_TAG_LEN + 16)
  uint8_t enc_buf[4 * FIO___TLS13_ENC_RECORD_SIZE];
  /* Partial write tracking: encrypted data that couldn't be fully sent.
   * TLS records are atomic - we must buffer partial writes because:
   * 1. Re-encrypting would use a new sequence number, corrupting the stream
   * 2. The browser would receive a partial record followed by a new record */
  size_t enc_buf_len;  /* Total encrypted data length in enc_buf */
  size_t enc_buf_sent; /* Bytes already sent from enc_buf */
  /* Parent context (for certificate chain) */
  fio___tls13_context_s *ctx;
  /* Certificate chain storage (for server - pointers must outlive handshake) */
  const uint8_t *cert_ptr;
  size_t cert_len;
  /* Flexible array member: all buffers in single allocation
   * Layout: [recv_buf | app_buf | send_buf], each FIO_IO_BUFFER_PER_WRITE */
  uint8_t buf[];
} fio___tls13_connection_s;

/** Buffer capacity per buffer region (recv, app, send) */
#define FIO___TLS13_BUF_CAP FIO_IO_BUFFER_PER_WRITE

/** Total buffer size for flexible array member */
#define FIO___TLS13_BUF_TOTAL (FIO___TLS13_BUF_CAP * 3)

/** Get pointer to recv_buf region */
FIO_IFUNC uint8_t *fio___tls13_recv_buf(fio___tls13_connection_s *c) {
  return c->buf + (FIO___TLS13_BUF_CAP * 0);
}

/** Get pointer to app_buf region */
FIO_IFUNC uint8_t *fio___tls13_app_buf(fio___tls13_connection_s *c) {
  return c->buf + (FIO___TLS13_BUF_CAP * 1);
}

/** Get pointer to send_buf region */
FIO_IFUNC uint8_t *fio___tls13_send_buf(fio___tls13_connection_s *c) {
  return c->buf + (FIO___TLS13_BUF_CAP * 2);
}

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

/** Callback for iterating ALPN protocols in fio_io_tls_s */
FIO_SFUNC int fio___tls13_each_alpn(struct fio_io_tls_each_s *e,
                                    const char *protocol_name,
                                    void (*on_selected)(fio_io_s *)) {
  fio___tls13_context_s *ctx = (fio___tls13_context_s *)e->udata;
  if (!protocol_name || !protocol_name[0])
    return 0;

  size_t len = FIO_STRLEN(protocol_name);
  if (len == 0 || len > 255) {
    FIO_LOG_ERROR("TLS 1.3: ALPN protocol name invalid length: %zu", len);
    return -1;
  }

  /* Append to comma-separated list */
  size_t needed = len + (ctx->alpn_protocols_len > 0 ? 1 : 0);
  if (ctx->alpn_protocols_len + needed >= sizeof(ctx->alpn_protocols)) {
    FIO_LOG_ERROR("TLS 1.3: ALPN protocol list overflow");
    return -1;
  }

  if (ctx->alpn_protocols_len > 0) {
    ctx->alpn_protocols[ctx->alpn_protocols_len++] = ',';
  }
  FIO_MEMCPY(ctx->alpn_protocols + ctx->alpn_protocols_len, protocol_name, len);
  ctx->alpn_protocols_len += len;
  ctx->alpn_protocols[ctx->alpn_protocols_len] = '\0';

  FIO_LOG_DDEBUG("TLS 1.3: added ALPN protocol: %s", protocol_name);
  (void)on_selected;
  return 0;
}

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
      fio_memcpy32(ctx->private_key, pkey.ecdsa_p256.private_key);
      if (pkey.ecdsa_p256.has_public_key) {
        FIO_MEMCPY(ctx->public_key, pkey.ecdsa_p256.public_key, 65);
      } else {
        /* Derive public key from private key */
#if defined(H___FIO_P256___H)
        uint8_t tmp_secret[32];
        fio_memcpy32(tmp_secret, pkey.ecdsa_p256.private_key);
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
      fio_memcpy32(ctx->private_key, pkey.ed25519.private_key);
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
    fio_memcpy32(ctx->private_key, fio___tls13_self_signed_keypair.secret_key);
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

#if defined(H___FIO_X509___H) && defined(H___FIO_PEM___H)

/** Internal: append one owned DER buffer to the context's trust store arrays.
 * Takes ownership of der_buf (allocated with FIO_MEM_REALLOC, size
 * der_buf_cap). Returns 0 on success, -1 on allocation failure (der_buf is
 * freed on error). */
FIO_SFUNC int fio___tls13_add_der_to_trust(fio___tls13_context_s *ctx,
                                           uint8_t *der_buf,
                                           size_t der_buf_cap,
                                           size_t der_len) {
  size_t new_count = ctx->trust_der_count + 1;
  uint8_t **new_bufs =
      (uint8_t **)FIO_MEM_REALLOC(ctx->trust_der_bufs,
                                  ctx->trust_der_count * sizeof(uint8_t *),
                                  new_count * sizeof(uint8_t *),
                                  ctx->trust_der_count * sizeof(uint8_t *));
  size_t *new_lens =
      (size_t *)FIO_MEM_REALLOC(ctx->trust_der_lens,
                                ctx->trust_der_count * sizeof(size_t),
                                new_count * sizeof(size_t),
                                ctx->trust_der_count * sizeof(size_t));
  if (!new_bufs || !new_lens) {
    FIO_LOG_ERROR("TLS 1.3: failed to allocate trust store arrays");
    FIO_MEM_FREE(der_buf, der_buf_cap);
    if (new_bufs)
      ctx->trust_der_bufs = new_bufs;
    if (new_lens)
      ctx->trust_der_lens = new_lens;
    return -1;
  }
  ctx->trust_der_bufs = new_bufs;
  ctx->trust_der_lens = new_lens;
  ctx->trust_der_bufs[ctx->trust_der_count] = der_buf;
  ctx->trust_der_lens[ctx->trust_der_count] = der_len;
  ctx->trust_der_count = new_count;
  /* Update trust_store to point to the (possibly reallocated) arrays */
  ctx->trust_store.roots = (const uint8_t **)ctx->trust_der_bufs;
  ctx->trust_store.root_lens = ctx->trust_der_lens;
  ctx->trust_store.root_count = ctx->trust_der_count;
  (void)der_buf_cap;
  return 0;
}

/** Internal: load all CERTIFICATE blocks from a PEM bundle into the trust
 * store. Iterates through all certs in the bundle (CA bundles contain
 * hundreds). Returns the number of certificates loaded (>= 0), or -1 on fatal
 * error. */
FIO_SFUNC int fio___tls13_load_pem_bundle(fio___tls13_context_s *ctx,
                                          const char *pem_data,
                                          size_t pem_len) {
  int loaded = 0;
  const char *pos = pem_data;
  size_t remaining = pem_len;

  while (remaining > 0) {
    /* Allocate a per-cert DER buffer (conservative: PEM is base64, DER ~75%) */
    size_t der_buf_cap = remaining;
    uint8_t *der_buf = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, der_buf_cap, 0);
    if (!der_buf) {
      FIO_LOG_ERROR("TLS 1.3: failed to allocate DER buffer for PEM bundle");
      return (loaded > 0) ? loaded : -1;
    }

    fio_pem_s pem_block;
    size_t consumed =
        fio_pem_parse(&pem_block, der_buf, der_buf_cap, pos, remaining);
    if (consumed == 0) {
      /* No more PEM blocks found */
      FIO_MEM_FREE(der_buf, der_buf_cap);
      break;
    }

    /* Only accept CERTIFICATE blocks (skip PRIVATE KEY etc.) */
    if (pem_block.label_len == 11 &&
        FIO_MEMCMP(pem_block.label, "CERTIFICATE", 11) == 0 &&
        pem_block.der_len > 0) {
      if (fio___tls13_add_der_to_trust(ctx,
                                       der_buf,
                                       der_buf_cap,
                                       pem_block.der_len) != 0)
        return (loaded > 0) ? loaded : -1; /* der_buf freed by helper */
      ++loaded;
    } else {
      FIO_MEM_FREE(der_buf, der_buf_cap);
    }

    pos += consumed;
    remaining -= consumed;
  }
  return loaded;
}

/** Internal: attempt to load the system CA trust store.
 * Tries platform-specific paths in order; loads the first one found.
 * Returns the number of CA certs loaded (>= 1) on success, 0 if not found. */
FIO_SFUNC int fio___tls13_load_system_trust(fio___tls13_context_s *ctx) {
#if defined(_WIN32)
  /* Windows: enumerate the "ROOT" system certificate store via CryptoAPI */
#include <wincrypt.h>
#ifdef _MSC_VER
#pragma comment(lib, "Crypt32.lib")
#endif
  HCERTSTORE hStore = CertOpenSystemStoreA(0, "ROOT");
  if (!hStore) {
    FIO_LOG_ERROR("TLS 1.3: failed to open Windows ROOT certificate store");
    return 0;
  }
  int loaded = 0;
  PCCERT_CONTEXT pCert = NULL;
  while ((pCert = CertEnumCertificatesInStore(hStore, pCert)) != NULL) {
    if (pCert->dwCertEncodingType != X509_ASN_ENCODING ||
        pCert->cbCertEncoded == 0)
      continue;
    size_t der_len = (size_t)pCert->cbCertEncoded;
    uint8_t *der_buf = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, der_len, 0);
    if (!der_buf)
      continue;
    FIO_MEMCPY(der_buf, pCert->pbCertEncoded, der_len);
    if (fio___tls13_add_der_to_trust(ctx, der_buf, der_len, der_len) == 0)
      ++loaded;
  }
  CertCloseStore(hStore, 0);
  if (loaded > 0)
    FIO_LOG_DEBUG2("TLS 1.3: loaded %d CA certs from Windows ROOT store",
                   loaded);
  return loaded;

#else  /* POSIX */
  /* Try platform-specific CA bundle paths in order */
  static const char *const ca_paths[] = {
      "/etc/ssl/cert.pem",                  /* macOS, FreeBSD */
      "/etc/ssl/certs/ca-certificates.crt", /* Debian/Ubuntu */
      "/etc/pki/tls/certs/ca-bundle.crt",   /* RHEL/CentOS/Fedora */
      "/etc/ssl/ca-bundle.pem",             /* openSUSE */
      "/etc/pki/ca-trust/extracted/pem/tls-ca-bundle.pem", /* RHEL newer */
      "/usr/local/etc/ssl/cert.pem",                       /* FreeBSD ports */
      NULL,
  };

  for (int i = 0; ca_paths[i] != NULL; ++i) {
    char *pem = fio_bstr_readfile(NULL, ca_paths[i], 0, 0);
    if (!pem)
      continue;

    size_t pem_len = fio_bstr_len(pem);
    int loaded = fio___tls13_load_pem_bundle(ctx, pem, pem_len);
    fio_bstr_free(pem);

    if (loaded > 0) {
      FIO_LOG_DEBUG2("TLS 1.3: loaded %d CA certs from system store: %s",
                     loaded,
                     ca_paths[i]);
      return loaded;
    }
  }
  return 0; /* No system store found */
#endif /* _WIN32 */
}

/** Callback for iterating trust certificates in fio_io_tls_s.
 *
 * Loads each PEM trust certificate into the context's trust store.
 * Called with public_cert_file == NULL when trust_sys is set — loads the
 * system CA trust store automatically. */
FIO_SFUNC int fio___tls13_each_trust(struct fio_io_tls_each_s *e,
                                     const char *public_cert_file) {
  fio___tls13_context_s *ctx = (fio___tls13_context_s *)e->udata;

  if (!public_cert_file) {
    /* System trust store requested: load platform CA bundle */
    int loaded = fio___tls13_load_system_trust(ctx);
    if (loaded <= 0) {
      FIO_LOG_ERROR("TLS 1.3: system trust store not found or empty — "
                    "cannot verify server certificates. Add CA certificates "
                    "explicitly via fio_io_tls_trust_add().");
      return -1;
    }
    return 0;
  }

  /* Read PEM file */
  char *pem = fio_bstr_readfile(NULL, public_cert_file, 0, 0);
  if (!pem) {
    FIO_LOG_ERROR("TLS 1.3: failed to read trust certificate: %s",
                  public_cert_file);
    return -1;
  }

  size_t pem_len = fio_bstr_len(pem);
  int loaded = fio___tls13_load_pem_bundle(ctx, pem, pem_len);
  fio_bstr_free(pem);

  if (loaded <= 0) {
    FIO_LOG_ERROR("TLS 1.3: failed to parse trust certificate PEM: %s",
                  public_cert_file);
    return -1;
  }

  FIO_LOG_DEBUG2("TLS 1.3: loaded %d trust certificate(s) from %s",
                 loaded,
                 public_cert_file);
  return 0;
}
#endif /* H___FIO_X509___H && H___FIO_PEM___H */

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
#if defined(H___FIO_X509___H) && defined(H___FIO_PEM___H)
    /* Build trust store from configured trust certificates.
     * When no explicit trust config exists, load the system CA store as the
     * secure default — this prevents silent MITM when the caller forgets to
     * configure trust anchors. */
    if (fio_io_tls_trust_count(tls) || tls->trust_sys) {
      /* Explicit trust config: load via iterator (handles both file certs and
       * trust_sys → system store via fio___tls13_each_trust NULL path) */
      if (fio_io_tls_each(tls,
                          .udata = ctx,
                          .each_trust = fio___tls13_each_trust)) {
        FIO_LOG_ERROR("TLS 1.3: failed to load trust certificates");
        goto error;
      }
    } else {
      /* No explicit trust config: load system CA store as secure default */
      int loaded = fio___tls13_load_system_trust(ctx);
      if (loaded <= 0) {
        FIO_LOG_ERROR("TLS 1.3: no trust store configured and system CA store "
                      "not found — TLS client connections will fail. Add CA "
                      "certificates via fio_io_tls_trust_add() or call "
                      "fio_io_tls_trust_add(tls, NULL) for system store.");
        goto error;
      }
      FIO_LOG_DEBUG2("TLS 1.3: using system CA store (%d certs) as default "
                     "trust anchor",
                     loaded);
    }
#endif     /* H___FIO_X509___H && H___FIO_PEM___H */
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
      fio_memcpy32(ctx->private_key,
                   fio___tls13_self_signed_keypair.secret_key);
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

    /* Collect ALPN protocols for server */
    if (fio_io_tls_alpn_count(tls)) {
      if (fio_io_tls_each(tls,
                          .udata = ctx,
                          .each_alpn = fio___tls13_each_alpn)) {
        FIO_LOG_ERROR("TLS 1.3: failed to configure ALPN protocols");
        goto error;
      }
      if (ctx->alpn_protocols_len > 0) {
        FIO_LOG_DEBUG2("TLS 1.3: ALPN protocols configured: %s",
                       ctx->alpn_protocols);
      }
    }
  }

  return ctx;

error:
  if (ctx) {
    if (ctx->cert_der)
      FIO_MEM_FREE(ctx->cert_der, ctx->cert_der_len);
#if defined(H___FIO_X509___H) && defined(H___FIO_PEM___H)
    for (size_t i = 0; i < ctx->trust_der_count; ++i)
      FIO_MEM_FREE(ctx->trust_der_bufs[i], ctx->trust_der_lens[i]);
    if (ctx->trust_der_bufs)
      FIO_MEM_FREE(ctx->trust_der_bufs,
                   ctx->trust_der_count * sizeof(uint8_t *));
    if (ctx->trust_der_lens)
      FIO_MEM_FREE(ctx->trust_der_lens, ctx->trust_der_count * sizeof(size_t));
#endif /* H___FIO_X509___H && H___FIO_PEM___H */
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
#if defined(H___FIO_X509___H) && defined(H___FIO_PEM___H)
  for (size_t i = 0; i < ctx->trust_der_count; ++i)
    FIO_MEM_FREE(ctx->trust_der_bufs[i], ctx->trust_der_lens[i]);
  if (ctx->trust_der_bufs)
    FIO_MEM_FREE(ctx->trust_der_bufs, ctx->trust_der_count * sizeof(uint8_t *));
  if (ctx->trust_der_lens)
    FIO_MEM_FREE(ctx->trust_der_lens, ctx->trust_der_count * sizeof(size_t));
#endif /* H___FIO_X509___H && H___FIO_PEM___H */
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

/** Allocate connection state with flexible array buffer */
FIO_SFUNC fio___tls13_connection_s *fio___tls13_connection_new(
    fio___tls13_context_s *ctx) {
  /* Single allocation for struct + all buffers (flex array member) */
  size_t alloc_size = sizeof(fio___tls13_connection_s) + FIO___TLS13_BUF_TOTAL;
  fio___tls13_connection_s *conn =
      (fio___tls13_connection_s *)FIO_MEM_REALLOC(NULL, 0, alloc_size, 0);
  if (!conn)
    return NULL;
  FIO_LEAK_COUNTER_ON_ALLOC(fio___tls13_connection_s);
  FIO_MEMSET(conn, 0, alloc_size);
  conn->is_client = ctx->is_client;
  conn->ctx = ctx;

  return conn;
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

  /* Single free for struct + all buffers (flex array member) */
  FIO_LEAK_COUNTER_ON_FREE(fio___tls13_connection_s);
  FIO_MEM_FREE(conn, sizeof(*conn) + FIO___TLS13_BUF_TOTAL);
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

    /* Configure certificate verification.
     * The trust store is populated by fio___tls13_build_context (either from
     * explicit certs or the system CA store).  If X509/PEM modules are absent,
     * verification will fail at handshake time unless skip_cert_verify is set
     * explicitly by the caller. */
#if defined(H___FIO_X509___H) && defined(H___FIO_PEM___H)
    if (ctx->trust_store.root_count > 0) {
      fio_tls13_client_set_trust_store(&conn->state.client, &ctx->trust_store);
    }
    /* No else: if trust_store is empty here, fio___tls13_build_context already
     * failed or the caller explicitly set skip_cert_verify elsewhere. */
#endif /* H___FIO_X509___H && H___FIO_PEM___H */

    /* Generate ClientHello */
    int ch_len = fio_tls13_client_start(&conn->state.client,
                                        fio___tls13_send_buf(conn),
                                        FIO___TLS13_BUF_CAP);
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

    /* Set ALPN protocols if configured */
    if (ctx->alpn_protocols_len > 0) {
      fio_tls13_server_alpn_set(&conn->state.server, ctx->alpn_protocols);
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
    FIO_MEMCPY(buf, fio___tls13_app_buf(conn) + conn->app_buf_pos, to_copy);
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
  size_t recv_space = FIO___TLS13_BUF_CAP - conn->recv_buf_len;

  /* Lazy compaction: only compact when buffer is >50% consumed AND
   * we don't have enough space for a new record */
  if (conn->recv_buf_pos > (FIO___TLS13_BUF_CAP >> 1) &&
      recv_space < FIO_TLS13_MAX_CIPHERTEXT_LEN) {
    if (recv_data_len > 0) {
      FIO_MEMMOVE(fio___tls13_recv_buf(conn),
                  fio___tls13_recv_buf(conn) + conn->recv_buf_pos,
                  recv_data_len);
    }
    conn->recv_buf_len = recv_data_len;
    conn->recv_buf_pos = 0;
    recv_space = FIO___TLS13_BUF_CAP - conn->recv_buf_len;
  }

  /* Read raw data from socket */
  errno = 0;
  ssize_t raw_read =
      fio_sock_read(fd,
                    (char *)fio___tls13_recv_buf(conn) + conn->recv_buf_len,
                    recv_space);
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
  uint8_t *recv_ptr = fio___tls13_recv_buf(conn) + conn->recv_buf_pos;
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
        if (conn->send_buf_len + out_len <= FIO___TLS13_BUF_CAP) {
          FIO_MEMCPY(fio___tls13_send_buf(conn) + conn->send_buf_len,
                     out_buf,
                     out_len);
          conn->send_buf_len += out_len;
        }
        /* Immediately try to send handshake response */
        while (conn->send_buf_pos < conn->send_buf_len) {
          errno = 0;
          ssize_t hs_written = fio_sock_write(
              fd,
              (char *)fio___tls13_send_buf(conn) + conn->send_buf_pos,
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
        decrypt_target = fio___tls13_app_buf(conn) + conn->app_buf_len;
        decrypt_capacity = FIO___TLS13_BUF_CAP - conn->app_buf_len;
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
    FIO_MEMCPY(buf, fio___tls13_app_buf(conn) + conn->app_buf_pos, to_copy);
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

/** Called to perform a non-blocking `write`, same as POSIX write(2).
 *
 * Returns:
 *   N > 0  - number of plaintext bytes accepted/encrypted
 *   0      - nothing to write (EOF condition, len was 0)
 *   -1     - error or EWOULDBLOCK (socket full, can't accept data now)
 *
 * Multi-record batching: encrypts up to 4 TLS records (64KB plaintext) into
 * enc_buf in a single call, then writes everything with one fio_sock_write().
 * Handshake data (send_buf) and KeyUpdate responses are prepended to enc_buf
 * so all outgoing data goes in a single syscall.
 *
 * IMPORTANT: Once data is encrypted, the TLS sequence number is incremented.
 * The function MUST return success (N > 0) after encryption, even if the
 * socket write fails. The encrypted data is buffered and flush() sends later.
 */
FIO_SFUNC ssize_t fio___tls13_write(int fd,
                                    const void *buf,
                                    size_t len,
                                    void *tls_ctx) {
  fio___tls13_connection_s *conn = (fio___tls13_connection_s *)tls_ctx;
  if (!conn || !buf) {
    errno = EINVAL;
    return -1;
  }
  if (len == 0)
    return 0;

  /* If handshake not complete, can't send application data */
  if (!conn->handshake_complete) {
    errno = EWOULDBLOCK;
    return -1;
  }

  /* CRITICAL: Flush any pending encrypted data from a previous partial write.
   * TLS records are atomic - we cannot re-encrypt because the sequence number
   * has already been incremented. We must send the buffered encrypted data
   * before we can accept new plaintext.
   *
   * IMPORTANT: We must NOT return the old plaintext_len here! The IO layer
   * passed us NEW data in buf/len. If we return the old length, the IO layer
   * will advance the stream by that amount, losing the new data.
   * Instead, we flush the pending data and then continue to process the new
   * data. If we can't fully flush, return EWOULDBLOCK. */
  while (conn->enc_buf_sent < conn->enc_buf_len) {
    size_t remaining = conn->enc_buf_len - conn->enc_buf_sent;
    errno = 0;
    ssize_t written = fio_sock_write(fd,
                                     (char *)conn->enc_buf + conn->enc_buf_sent,
                                     remaining);
    if (written <= 0) {
      if (errno == EWOULDBLOCK || errno == EAGAIN) {
        errno = EWOULDBLOCK;
        return -1;
      }
      return -1; /* Real socket error */
    }
    conn->enc_buf_sent += (size_t)written;
  }

  /* Pending data fully sent - reset buffer */
  conn->enc_buf_len = 0;
  conn->enc_buf_sent = 0;

  /* Position in enc_buf where we'll write next */
  size_t enc_pos = 0;

  /* Copy any pending handshake data (e.g., client Finished) into enc_buf
   * so it goes out in the same syscall as application data. */
  if (conn->send_buf_pos < conn->send_buf_len) {
    size_t hs_remaining = conn->send_buf_len - conn->send_buf_pos;
    if (hs_remaining <= sizeof(conn->enc_buf)) {
      FIO_MEMCPY(conn->enc_buf,
                 fio___tls13_send_buf(conn) + conn->send_buf_pos,
                 hs_remaining);
      enc_pos = hs_remaining;
    }
    conn->send_buf_len = 0;
    conn->send_buf_pos = 0;
  }

  /* RFC 8446 Section 4.6.3: Send KeyUpdate response before Application Data
   * if one is pending. Encrypt into enc_buf with OLD sending keys,
   * then update our sending keys. */
  if (conn->is_client && conn->state.client.key_update_pending) {
    size_t ku_space = sizeof(conn->enc_buf) - enc_pos;
    size_t key_len = fio___tls13_key_len(&conn->state.client);
    fio_tls13_cipher_type_e cipher_type =
        fio___tls13_cipher_type(&conn->state.client);

    int ku_len = fio_tls13_send_key_update_response(
        conn->enc_buf + enc_pos,
        ku_space,
        conn->state.client.client_app_traffic_secret,
        &conn->state.client.client_app_keys,
        &conn->state.client.key_update_pending,
        conn->state.client.use_sha384,
        key_len,
        cipher_type);

    if (ku_len > 0) {
      enc_pos += (size_t)ku_len;
      FIO_LOG_DEBUG2("TLS 1.3 Client: KeyUpdate response queued in enc_buf");
    }
  } else if (!conn->is_client && conn->state.server.key_update_pending) {
    size_t ku_space = sizeof(conn->enc_buf) - enc_pos;
    size_t key_len = fio___tls13_server_key_len(&conn->state.server);
    fio_tls13_cipher_type_e cipher_type =
        fio___tls13_server_cipher_type(&conn->state.server);

    int ku_len = fio_tls13_send_key_update_response(
        conn->enc_buf + enc_pos,
        ku_space,
        conn->state.server.server_app_traffic_secret,
        &conn->state.server.server_app_keys,
        &conn->state.server.key_update_pending,
        conn->state.server.use_sha384,
        key_len,
        cipher_type);

    if (ku_len > 0) {
      enc_pos += (size_t)ku_len;
      FIO_LOG_DEBUG2("TLS 1.3 Server: KeyUpdate response queued in enc_buf");
    }
  }

  /* Multi-record encryption: loop encrypting up to 4 TLS records into enc_buf.
   * Each record holds up to FIO_TLS13_MAX_PLAINTEXT_LEN (16384) bytes. */
  size_t total_plaintext = 0;
  const uint8_t *src = (const uint8_t *)buf;
  size_t remaining_plaintext = len;

  while (remaining_plaintext > 0) {
    /* Check if enc_buf has space for at least one more max record */
    size_t enc_space = sizeof(conn->enc_buf) - enc_pos;
    if (enc_space < FIO_TLS13_RECORD_HEADER_LEN + 1 + 1 + FIO_TLS13_TAG_LEN)
      break; /* Not enough space for even a minimal record */

    /* Clamp this chunk to max plaintext per record */
    size_t chunk = remaining_plaintext;
    if (chunk > FIO_TLS13_MAX_PLAINTEXT_LEN)
      chunk = FIO_TLS13_MAX_PLAINTEXT_LEN;

    /* Also clamp to what fits in remaining enc_buf space:
     * encrypted size = header(5) + plaintext + content_type(1) + tag(16) */
    size_t max_pt_for_space =
        enc_space - FIO_TLS13_RECORD_HEADER_LEN - 1 - FIO_TLS13_TAG_LEN;
    if (chunk > max_pt_for_space)
      chunk = max_pt_for_space;

    /* Encrypt one record into enc_buf at enc_pos */
    int enc_len;
    if (conn->is_client) {
      enc_len = fio_tls13_client_encrypt(&conn->state.client,
                                         conn->enc_buf + enc_pos,
                                         enc_space,
                                         src,
                                         chunk);
    } else {
      enc_len = fio_tls13_server_encrypt(&conn->state.server,
                                         conn->enc_buf + enc_pos,
                                         enc_space,
                                         src,
                                         chunk);
    }

    if (enc_len < 0) {
      /* Encryption error. If we already encrypted some records, return what
       * we have. Otherwise report error. */
      if (total_plaintext > 0)
        break;
      FIO_LOG_DEBUG2("TLS 1.3: encryption error");
      errno = ECONNRESET;
      return -1;
    }

    enc_pos += (size_t)enc_len;
    src += chunk;
    remaining_plaintext -= chunk;
    total_plaintext += chunk;
  }

  /* If we have data to send (handshake, KeyUpdate, and/or encrypted records),
   * write it all in a single syscall. */
  if (enc_pos > 0) {
    errno = 0;
    ssize_t written = fio_sock_write(fd, (char *)conn->enc_buf, enc_pos);
    if (written <= 0) {
      if (errno == EWOULDBLOCK || errno == EAGAIN) {
        /* Socket buffer full. Buffer everything for later.
         * CRITICAL: Return SUCCESS for any encrypted plaintext because
         * sequence numbers are incremented. flush() will send later. */
        conn->enc_buf_len = enc_pos;
        conn->enc_buf_sent = 0;
        errno = 0;
        return total_plaintext > 0 ? (ssize_t)total_plaintext : -1;
      }
      return -1; /* Real socket error */
    }

    /* Partial write - buffer the remainder.
     * CRITICAL: Cannot re-encrypt; sequence numbers already incremented. */
    if ((size_t)written < enc_pos) {
      conn->enc_buf_len = enc_pos;
      conn->enc_buf_sent = (size_t)written;
    }
  }

  /* Report total plaintext bytes accepted.
   * N > 0: plaintext was encrypted and sent/buffered.
   * -1: no plaintext could be encrypted (enc_buf full of control data). */
  if (total_plaintext > 0)
    return (ssize_t)total_plaintext;
  errno = EWOULDBLOCK;
  return -1;
}

/* *****************************************************************************
TLS 1.3 IO Functions - Flush
***************************************************************************** */

/** Sends any unsent internal data, returning POSIX write(2) semantics.
 *
 * Returns:
 *   N > 0  - number of bytes written to the socket
 *   0      - nothing to flush, all internal buffers are empty (EOF)
 *   -1     - error or EWOULDBLOCK (pending data couldn't be flushed)
 *
 * After the write() rewrite, all outgoing data (handshake, KeyUpdate,
 * encrypted records) is unified into enc_buf. Flush only needs to drain it.
 * The send_buf is still flushed here for handshake data generated during
 * read() (before write() has a chance to copy it into enc_buf). */
FIO_SFUNC int fio___tls13_flush(int fd, void *tls_ctx) {
  fio___tls13_connection_s *conn = (fio___tls13_connection_s *)tls_ctx;
  if (!conn)
    return 0;

  size_t total_flushed = 0;

  /* Send any buffered handshake data that was generated during read()
   * and hasn't been copied into enc_buf by write() yet. */
  while (conn->send_buf_pos < conn->send_buf_len) {
    errno = 0;
    ssize_t written =
        fio_sock_write(fd,
                       (char *)fio___tls13_send_buf(conn) + conn->send_buf_pos,
                       conn->send_buf_len - conn->send_buf_pos);
    if (written <= 0) {
      if (total_flushed > 0)
        goto done;
      return -1;
    }
    conn->send_buf_pos += (size_t)written;
    total_flushed += (size_t)written;
  }

  /* Reset send buffer if fully sent */
  if (conn->send_buf_pos >= conn->send_buf_len) {
    conn->send_buf_len = 0;
    conn->send_buf_pos = 0;
  }

  /* Flush pending encrypted data (handshake + KeyUpdate + app records) */
  while (conn->enc_buf_sent < conn->enc_buf_len) {
    size_t remaining = conn->enc_buf_len - conn->enc_buf_sent;
    errno = 0;
    ssize_t written = fio_sock_write(fd,
                                     (char *)conn->enc_buf + conn->enc_buf_sent,
                                     remaining);
    if (written <= 0) {
      if (total_flushed > 0)
        goto done;
      return -1;
    }
    conn->enc_buf_sent += (size_t)written;
    total_flushed += (size_t)written;
  }

  /* Reset encrypted buffer if fully sent */
  if (conn->enc_buf_sent >= conn->enc_buf_len) {
    conn->enc_buf_len = 0;
    conn->enc_buf_sent = 0;
  }

done:
  /* N > 0: bytes were flushed (may still have pending data).
   * 0: all internal buffers are empty.
   * The IO layer uses truthiness to decide whether to monitor for POLLOUT,
   * so both N > 0 and -1 correctly trigger continued monitoring. */
  if (total_flushed > 0)
    return (int)total_flushed;
  /* Check if there's still pending data we couldn't flush */
  if ((conn->send_buf_len > conn->send_buf_pos) ||
      (conn->enc_buf_len > conn->enc_buf_sent)) {
    errno = EWOULDBLOCK;
    return -1;
  }
  return 0;
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
      int ignr_ = fio_sock_write(fd, (char *)alert, (size_t)enc_len);
      (void)ignr_;
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
