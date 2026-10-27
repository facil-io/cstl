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

#if defined(_WIN32)
/* wincrypt.h must be included at file scope - MSVC rejects #include inside a
 * function body (error C2071: illegal storage class). */
#include <wincrypt.h>
#ifdef _MSC_VER
#pragma comment(lib, "Crypt32.lib")
#endif
#endif /* _WIN32 */

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

/** A server certificate + key (same field names as the context's default) */
typedef struct {
  fio_ubuf_info_s *cert_chain; /* Owned array of owned DER views */
  size_t cert_chain_count;
  uint8_t private_key[48];
  uint8_t public_key[65];
  size_t private_key_len;
  uint16_t signature_algo;
  fio_ubuf_info_s private_key_ext;
} fio___tls13_cred_s;

/** Context for all future connections (built from fio_io_tls_s) */
typedef struct {
  fio_io_tls_s *tls;
  /* References: one per `build_context` / `dup_context` result (released by
   * free_context) plus one per connection (released by cleanup; `start`
   * releases the IO's reference). Connections borrow the server name,
   * trust store, certificate chain and keys below, so the context must
   * outlive every connection built from it (e.g., after a listener stops or
   * a client's connecting state is released mid-handshake). */
  size_t ref;
  uint8_t is_client;
  /* Certificate chain (DER-encoded), presented by servers and (when
   * requested) by clients */
  fio_ubuf_info_s cert_der;    /* alias for first certificate (compat/debug) */
  fio_ubuf_info_s *cert_chain; /* Owned array of DER views */
  size_t cert_chain_count;
  /* Private key (P-256: 32-byte scalar, P-384: 48-byte scalar, Ed25519:
   * 32-byte seed). RSA keys are too large for this buffer; use
   * private_key_ext instead. */
  uint8_t private_key[48];
  /* Public key for P-256 signing (65 bytes: 0x04 || x || y) */
  uint8_t public_key[65];
  size_t private_key_len;
  uint16_t signature_algo;
  /* Extended private key buffer for RSA (the length-prefixed n, d, e, p, q,
   * dP, dQ, qInv format of the TLS layer). */
  fio_ubuf_info_s private_key_ext;
  /* Output space reserved before each handshake step: enough for the
   * largest flight of any certificate chain below (fio_tls13_flight_size). */
  size_t flight_size;
  /* Server SNI selection (only when 2+ certificates are configured): every
   * certificate in `fio_io_tls_each` order, except the default one, which is
   * moved into the fields above (its `creds` slot stays empty). */
  fio___tls13_cred_s *creds;
  size_t creds_count;
  fio___io_tls_sni_index_s sni;
  /* SNI hostname for client connections */
  char server_name[256];
  /* ALPN protocols (comma-separated) for server */
  char alpn_protocols[256];
  size_t alpn_protocols_len;
#if defined(H___FIO_X509___H) && defined(H___FIO_PEM___H)
  /* Trust store for client certificate verification (NULL = skip verify) */
  fio_x509_trust_store_s trust_store;
  /* Owned DER buffers backing trust_store.roots.
   * trust_der_bufs/lens/count are NULL/0 when trust_store points to the
   * global system trust singleton (trust_der_owned == 0). In that case the
   * free path must NOT free the backing buffers — they are owned by the
   * global singleton. */
  uint8_t **trust_der_bufs;
  size_t *trust_der_lens;
  size_t trust_der_count;
  uint8_t trust_der_owned; /* 1 = context owns bufs, 0 = global singleton */
#endif                     /* H___FIO_X509___H && H___FIO_PEM___H */
} fio___tls13_context_s;

FIO_LEAK_COUNTER_DEF(fio___tls13_context_s)
/* every owned DER/PEM-decoded/key heap buffer (cert chain entries, trust
 * store entries, RSA key blobs, transient cert scratch) shares one counter:
 * a wrong `trust_der_owned` flag or an ownership-transfer bug shows up as a
 * double-free assert / leak report on this counter.
 * Note: the trust `bufs`/`lens` arrays themselves use a realloc-grow pattern
 * and are intentionally NOT counted (realloc is neither alloc nor free);
 * their entries are, which covers the dangerous double-free case. */
FIO_LEAK_COUNTER_DEF(fio___tls13_der_buf)
FIO_LEAK_COUNTER_DEF(fio___tls13_cert_chain_ary)

/** Free an owned DER certificate chain (entries and array). */
FIO_SFUNC void fio___tls13_chain_free(fio_ubuf_info_s *chain, size_t count) {
  for (size_t i = 0; i < count; ++i) {
    FIO_LEAK_COUNTER_ON_FREE(fio___tls13_der_buf);
    FIO_MEM_FREE((void *)chain[i].buf, chain[i].len);
  }
  if (chain) {
    FIO_LEAK_COUNTER_ON_FREE(fio___tls13_cert_chain_ary);
    FIO_MEM_FREE((void *)chain, count * sizeof(*chain));
  }
}

/** Free DER certificate buffers owned by a context. */
FIO_SFUNC void fio___tls13_context_cert_chain_free(fio___tls13_context_s *ctx) {
  if (!ctx)
    return;
  fio___tls13_chain_free(ctx->cert_chain, ctx->cert_chain_count);
  ctx->cert_der = (fio_ubuf_info_s){0};
  ctx->cert_chain = NULL;
  ctx->cert_chain_count = 0;
}

/** Add an owned DER certificate buffer to a context chain. */
FIO_SFUNC int fio___tls13_context_cert_chain_add(fio___tls13_context_s *ctx,
                                                 uint8_t *der_buf,
                                                 size_t der_len) {
  if (!ctx || !der_buf || !der_len)
    return -1;
  size_t old_count = ctx->cert_chain_count;
  size_t new_count = old_count + 1;
  fio_ubuf_info_s *new_chain = (fio_ubuf_info_s *)
      FIO_MEM_REALLOC(NULL, 0, new_count * sizeof(*ctx->cert_chain), 0);
  if (!new_chain) {
    FIO_LEAK_COUNTER_ON_FREE(fio___tls13_der_buf);
    FIO_MEM_FREE(der_buf, der_len);
    return -1;
  }
  FIO_LEAK_COUNTER_ON_ALLOC(fio___tls13_cert_chain_ary);
  if (old_count) {
    FIO_MEMCPY(new_chain,
               ctx->cert_chain,
               old_count * sizeof(*ctx->cert_chain));
    FIO_LEAK_COUNTER_ON_FREE(fio___tls13_cert_chain_ary);
    FIO_MEM_FREE((void *)ctx->cert_chain, old_count * sizeof(*ctx->cert_chain));
  }
  ctx->cert_chain = new_chain;
  ctx->cert_chain[old_count] = FIO_UBUF_INFO2(der_buf, der_len);
  ctx->cert_chain_count = new_count;
  if (!old_count) {
    ctx->cert_der = FIO_UBUF_INFO2(der_buf, der_len);
  }
  return 0;
}

/* *****************************************************************************
TLS 1.3 Server Credentials (SNI certificate selection)
***************************************************************************** */

/** Moves credential fields from `src` to `dst` (a context or a cred). */
#define FIO___TLS13_CRED_MOVE(dst, src)                                        \
  do {                                                                         \
    (dst)->cert_chain = (src)->cert_chain;                                     \
    (dst)->cert_chain_count = (src)->cert_chain_count;                         \
    FIO_MEMCPY((dst)->private_key,                                             \
               (src)->private_key,                                             \
               sizeof((dst)->private_key));                                    \
    FIO_MEMCPY((dst)->public_key, (src)->public_key, 65);                      \
    (dst)->private_key_len = (src)->private_key_len;                           \
    (dst)->signature_algo = (src)->signature_algo;                             \
    (dst)->private_key_ext = (src)->private_key_ext;                           \
    (src)->cert_chain = NULL;                                                  \
    (src)->cert_chain_count = 0;                                               \
    fio_secure_zero((src)->private_key, sizeof((src)->private_key));           \
    FIO_MEMSET((src)->public_key, 0, 65);                                      \
    (src)->private_key_len = 0;                                                \
    (src)->signature_algo = 0;                                                 \
    (src)->private_key_ext = (fio_ubuf_info_s){0};                             \
  } while (0)

/** The private key view of a credential (a context or a cred): the RSA
 * buffer or the inline EC / Ed25519 key, in the TLS 1.3 key format. */
#define FIO___TLS13_CRED_KEY(c)                                                \
  ((c)->private_key_ext.buf                                                    \
       ? (c)->private_key_ext                                                  \
       : FIO_UBUF_INFO2((c)->private_key, (c)->private_key_len))

/** Hands a credential (a context or a cred) to a server handshake. */
#define FIO___TLS13_CRED_APPLY(server, c)                                      \
  fio___tls13_server_cred_apply((server),                                      \
                                (c)->cert_chain,                               \
                                (c)->cert_chain_count,                         \
                                FIO___TLS13_CRED_KEY(c),                       \
                                (c)->public_key,                               \
                                (c)->signature_algo)

/** Sets a server handshake's certificate chain and private key (views). */
FIO_SFUNC void fio___tls13_server_cred_apply(fio_tls13_server_s *server,
                                             const fio_ubuf_info_s *chain,
                                             size_t chain_count,
                                             fio_ubuf_info_s key,
                                             const uint8_t *public_key,
                                             uint16_t signature_algo) {
  if (chain && chain_count)
    fio_tls13_server_set_cert_chain(server, chain, chain_count);
  if (key.len) {
    fio_tls13_server_set_private_key(
        server,
        key,
        (fio_tls13_signature_algo_e)signature_algo);
    /* Copy public key for P-256 signing */
    if (signature_algo == FIO_TLS13_SIGNATURE_ECDSA_SECP256R1_SHA256)
      FIO_MEMCPY(server->credentials.public_key, public_key, 65);
  }
}

/** Frees a credential (chain and keys). */
FIO_SFUNC void fio___tls13_cred_destroy(fio___tls13_cred_s *c) {
  fio___tls13_chain_free(c->cert_chain, c->cert_chain_count);
  if (c->private_key_ext.buf) {
    fio_secure_zero(c->private_key_ext.buf, c->private_key_ext.len);
    FIO_LEAK_COUNTER_ON_FREE(fio___tls13_der_buf);
    FIO_MEM_FREE(c->private_key_ext.buf, c->private_key_ext.len);
  }
  fio_secure_zero(c, sizeof(*c));
}

/** Frees the SNI selection data (non-default certificates and the index). */
FIO_SFUNC void fio___tls13_context_creds_free(fio___tls13_context_s *ctx) {
  for (size_t i = 0; i < ctx->creds_count; ++i)
    fio___tls13_cred_destroy(ctx->creds + i);
  if (ctx->creds)
    FIO_MEM_FREE(ctx->creds, ctx->creds_count * sizeof(*ctx->creds));
  ctx->creds = NULL;
  ctx->creds_count = 0;
  fio___io_tls_sni_destroy(&ctx->sni);
}

/** ClientHello callback: presents the certificate matching the SNI. */
FIO_SFUNC int fio___tls13_on_hello(fio_tls13_server_s *server,
                                   const char *sni,
                                   size_t sni_len,
                                   void *udata) {
  fio___tls13_context_s *ctx = (fio___tls13_context_s *)udata;
  size_t pos = fio___io_tls_sni_find(&ctx->sni, sni, sni_len);
  if (pos != ctx->sni.dflt && pos < ctx->creds_count) /* else: default set */
    FIO___TLS13_CRED_APPLY(server, ctx->creds + pos);
  return 0;
}

/* *****************************************************************************
TLS 1.3 Global System Trust Store Singleton
*
* The system CA bundle is large (128+ certs on macOS) and immutable.
* Loading it per-context (per fio_io_connect call) wastes memory linearly with
* N concurrent client connections.  Instead, load it once into this global
* singleton and share it (read-only) across all contexts.
*
* Lifetime: allocated at first use, freed at process exit via
* FIO_CALL_AT_EXIT callback.  Protected by fio___tls13_sys_trust_lock.
***************************************************************************** */
#if defined(H___FIO_X509___H) && defined(H___FIO_PEM___H)
typedef struct {
  fio_x509_trust_store_s store; /* roots/root_lens/root_count */
  uint8_t **bufs;               /* owned DER buffers (freed at exit) */
  size_t *lens;                 /* DER lengths (alloc size == content size) */
  size_t count;
  uint8_t loaded; /* 1 once successfully populated */
} fio___tls13_sys_trust_s;

static fio___tls13_sys_trust_s fio___tls13_sys_trust;
static fio_lock_i fio___tls13_sys_trust_lock = FIO_LOCK_INIT;

/** AT_EXIT callback: free the global system trust store.
 * Registered once at startup via FIO_CONSTRUCTOR below. */
FIO_SFUNC void fio___tls13_sys_trust_free(void *ignr_) {
  fio_lock(&fio___tls13_sys_trust_lock);
  for (size_t i = 0; i < fio___tls13_sys_trust.count; ++i) {
    FIO_LEAK_COUNTER_ON_FREE(fio___tls13_der_buf);
    FIO_MEM_FREE(fio___tls13_sys_trust.bufs[i], fio___tls13_sys_trust.lens[i]);
  }
  if (fio___tls13_sys_trust.bufs)
    FIO_MEM_FREE(fio___tls13_sys_trust.bufs,
                 fio___tls13_sys_trust.count * sizeof(uint8_t *));
  if (fio___tls13_sys_trust.lens)
    FIO_MEM_FREE(fio___tls13_sys_trust.lens,
                 fio___tls13_sys_trust.count * sizeof(size_t));
  fio___tls13_sys_trust = (fio___tls13_sys_trust_s){0};
  fio_unlock(&fio___tls13_sys_trust_lock);
  (void)ignr_;
}

/** Register the system trust cleanup exactly once at process start. */
FIO_CONSTRUCTOR(fio___tls13_sys_trust_register_cleanup) {
  fio_state_callback_add(FIO_CALL_AT_EXIT, fio___tls13_sys_trust_free, NULL);
}
#endif /* H___FIO_X509___H && H___FIO_PEM___H */

/** Per-connection TLS state.
 *
 * Hot metadata precedes all buffers, so common read/write bookkeeping stays
 * cache-local. The wire-output queue holds both plaintext handshake records
 * and encrypted post-handshake records. */
#define FIO___TLS13_OUT_RECORD_SIZE                                            \
  (FIO_TLS13_MAX_CIPHERTEXT_LEN + FIO_TLS13_RECORD_HEADER_LEN +                \
   FIO_TLS13_TAG_LEN + 16)
#define FIO___TLS13_OUT_BUF_CAP (4 * FIO___TLS13_OUT_RECORD_SIZE)
#define FIO___TLS13_RECV_BUF_CAP                                               \
  (FIO_TLS13_RECORD_HEADER_LEN + FIO_TLS13_MAX_CIPHERTEXT_LEN)
#define FIO___TLS13_APP_BUF_CAP FIO_TLS13_MAX_PLAINTEXT_LEN
/* KeyUpdate record: header + handshake header (4) + body (1) + type + tag */
#define FIO___TLS13_KEY_UPDATE_RECORD_LEN                                      \
  (FIO_TLS13_RECORD_HEADER_LEN + 4 + 1 + 1 + FIO_TLS13_TAG_LEN)
#define FIO___TLS13_READ_SCRATCH_CAP (4 * FIO___TLS13_RECV_BUF_CAP)
/* Output space reserved before each handshake step: the context's largest
 * flight, and never less than 8 KiB (ClientHello / HelloRetryRequest). */
#define FIO___TLS13_HS_OUT_MIN 8192
#define FIO___TLS13_HS_OUT_NEED(ctx)                                           \
  ((ctx)->flight_size > FIO___TLS13_HS_OUT_MIN ? (ctx)->flight_size            \
                                               : FIO___TLS13_HS_OUT_MIN)

typedef struct {
  /* Hot read/write cursors. */
  size_t out_buf_len;
  size_t out_buf_sent;
  /* Wire-output queue: `out_inline`, or an allocation while a handshake
   * flight larger than it is queued (see fio___tls13_out_reserve). */
  uint8_t *out_buf;
  size_t out_buf_cap;
  size_t recv_buf_len;
  size_t recv_buf_pos;
  size_t app_buf_len;
  size_t app_buf_pos;
  uint8_t is_client;
  uint8_t handshake_complete;
  uint8_t peer_closed;

  /* Connection metadata. */
  fio_io_s *io;
  fio___tls13_context_s *ctx;
  const uint8_t *cert_ptr;
  size_t cert_len;

  /* TLS session state. */
  union {
    fio_tls13_client_s client;
    fio_tls13_server_s server;
  } state;

  /* Serialized wire data, retained across partial socket writes. */
  uint8_t out_inline[FIO___TLS13_OUT_BUF_CAP];

  /* Flexible storage layout: [recv_buf | app_buf]. */
  uint8_t buf[];
} fio___tls13_connection_s;

#define FIO___TLS13_BUF_TOTAL                                                  \
  (FIO___TLS13_RECV_BUF_CAP + FIO___TLS13_APP_BUF_CAP)

/** Get pointer to the persistent ciphertext receive buffer. */
FIO_IFUNC uint8_t *fio___tls13_recv_buf(fio___tls13_connection_s *c) {
  return c->buf;
}

/** Get pointer to the persistent decrypted application buffer. */
FIO_IFUNC uint8_t *fio___tls13_app_buf(fio___tls13_connection_s *c) {
  return c->buf + FIO___TLS13_RECV_BUF_CAP;
}

FIO_LEAK_COUNTER_DEF(fio___tls13_connection_s)

FIO_STATIC_ALLOC_DEF(fio___tls13_buffer_alloc,
                     uint8_t,
                     FIO___TLS13_READ_SCRATCH_CAP,
                     4)

/* *****************************************************************************
TLS 1.3 Context Builder - Self-Signed Certificate Generation
*
* The self-signed certificate is generated once (lazily, under a lock) and
* stored directly in the context.  Its lifetime is tied to the context — freed
* in fio___tls13_free_context_task.  No global state, no state callbacks.
***************************************************************************** */

#if defined(H___FIO_X509___H)
/** Generate a self-signed P-256 certificate directly into ctx. */
FIO_SFUNC int fio___tls13_make_self_signed(fio___tls13_context_s *ctx,
                                           const char *server_name) {
  if (!server_name || !server_name[0])
    server_name = "localhost";

  FIO_LOG_DEBUG2("TLS 1.3: generating P-256 self-signed certificate...");

  fio_x509_keypair_s keypair;
  if (fio_x509_keypair_p256(&keypair) != 0) {
    FIO_LOG_ERROR("TLS 1.3: failed to generate P-256 keypair");
    return -1;
  }

  fio_buf_info_s san_dns[] = {
      FIO_BUF_INFO2((char *)server_name, FIO_STRLEN(server_name))};
  fio_x509_cert_options_s opts = {
      .cn = FIO_BUF_INFO2((char *)server_name, FIO_STRLEN(server_name)),
      .org = FIO_BUF_INFO2((char *)"facil.io", 8),
      .san_dns = san_dns,
      .san_dns_count = 1,
      .is_ca = 0,
  };

  size_t cert_size = fio_x509_self_signed_cert(NULL, 0, &keypair, &opts);
  if (cert_size == 0) {
    FIO_LOG_ERROR("TLS 1.3: failed to calculate self-signed certificate size");
    fio_x509_keypair_clear(&keypair);
    return -1;
  }

  uint8_t *cert_tmp = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, cert_size, 0);
  if (!cert_tmp) {
    FIO_LOG_ERROR("TLS 1.3: failed to allocate certificate buffer");
    fio_x509_keypair_clear(&keypair);
    return -1;
  }
  FIO_LEAK_COUNTER_ON_ALLOC(fio___tls13_der_buf);

  size_t cert_der_len =
      fio_x509_self_signed_cert(cert_tmp, cert_size, &keypair, &opts);
  if (cert_der_len == 0) {
    FIO_LOG_ERROR("TLS 1.3: failed to generate self-signed certificate");
    FIO_LEAK_COUNTER_ON_FREE(fio___tls13_der_buf);
    FIO_MEM_FREE(cert_tmp, cert_size);
    fio_x509_keypair_clear(&keypair);
    return -1;
  }

  uint8_t *cert_der = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, cert_der_len, 0);
  if (!cert_der) {
    FIO_LOG_ERROR("TLS 1.3: failed to allocate exact certificate buffer");
    FIO_LEAK_COUNTER_ON_FREE(fio___tls13_der_buf);
    FIO_MEM_FREE(cert_tmp, cert_size);
    fio_x509_keypair_clear(&keypair);
    return -1;
  }
  FIO_LEAK_COUNTER_ON_ALLOC(fio___tls13_der_buf);
  FIO_MEMCPY(cert_der, cert_tmp, cert_der_len);
  FIO_LEAK_COUNTER_ON_FREE(fio___tls13_der_buf);
  FIO_MEM_FREE(cert_tmp, cert_size);

  if (fio___tls13_context_cert_chain_add(ctx, cert_der, cert_der_len) != 0) {
    FIO_LOG_ERROR("TLS 1.3: failed to store self-signed certificate");
    fio_x509_keypair_clear(&keypair);
    return -1;
  }

  /* Store key material in context */
  fio_memcpy32(ctx->private_key, keypair.secret_key);
  FIO_MEMCPY(ctx->public_key, keypair.public_key, 65);
  ctx->private_key_len = 32;
  ctx->signature_algo = FIO_TLS13_SIGNATURE_ECDSA_SECP256R1_SHA256;

  fio_x509_keypair_clear(&keypair);
  FIO_LOG_DEBUG2("TLS 1.3: self-signed certificate generated (%zu bytes)",
                 ctx->cert_der.len);
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

#if defined(H___FIO_PEM___H) && defined(H___FIO_X509___H)
/** Wipes and frees a `fio_bstr` that held secret material (key files). */
FIO_SFUNC void fio___tls13_bstr_free_secret(char *s) {
  if (!s)
    return;
  fio_secure_zero(s, fio_bstr_len(s));
  fio_bstr_free(s);
}

/** Returns the next "-----BEGIN " marker in `data`, or NULL. */
FIO_SFUNC const char *fio___tls13_pem_begin(const char *data, size_t len) {
  static const char marker[] = "-----BEGIN ";
  const size_t mlen = sizeof(marker) - 1;
  const char *const end = data + len;
  while ((size_t)(end - data) >= mlen) {
    const char *dash =
        (const char *)FIO_MEMCHR(data, '-', (size_t)(end - data) - mlen + 1);
    if (!dash)
      return NULL;
    if (!FIO_MEMCMP(dash, marker, mlen))
      return dash;
    data = dash + 1;
  }
  return NULL;
}

/** PEM bundle reader: decodes into a stack buffer, falling back to a reused
 * heap buffer for blocks too large for it (e.g., DER > 16 KiB). */
typedef struct {
  const char *pos;
  size_t remaining;
  uint8_t *heap;
  size_t heap_len;
  uint8_t stack[16384];
} fio___tls13_pem_reader_s;

/**
 * Decodes the next PEM block into `blk` (valid until the next call).
 *
 * Returns 1 when a block was decoded, 0 when no further PEM block exists, or
 * -1 when the next block can't be decoded (malformed, or out of memory). On
 * -1 the reader advances past that block's BEGIN marker, so callers may skip
 * it and continue.
 */
FIO_SFUNC int fio___tls13_pem_read(fio___tls13_pem_reader_s *r,
                                   fio_pem_s *blk) {
  if (!r->remaining)
    return 0;
  size_t consumed =
      fio_pem_parse(blk, r->stack, sizeof(r->stack), r->pos, r->remaining);
  if (consumed) {
    r->pos += consumed;
    r->remaining -= consumed;
    return 1;
  }
  const char *begin = fio___tls13_pem_begin(r->pos, r->remaining);
  if (!begin) {
    r->pos += r->remaining;
    r->remaining = 0;
    return 0;
  }
  size_t left = r->remaining - (size_t)(begin - r->pos);
  /* Retry with a heap buffer: decoded DER is smaller than its Base64 armor */
  if (left > sizeof(r->stack) && r->heap_len < left) {
    if (r->heap)
      FIO_MEM_FREE(r->heap, r->heap_len);
    r->heap = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, left, 0);
    r->heap_len = r->heap ? left : 0;
  }
  if (left > sizeof(r->stack) && r->heap)
    consumed = fio_pem_parse(blk, r->heap, r->heap_len, begin, left);
  if (!consumed) {
    r->pos = begin + 11; /* skip this block's BEGIN marker */
    r->remaining = left - 11;
    return -1;
  }
  r->pos = begin + consumed;
  r->remaining = left - consumed;
  return 1;
}

/** Releases the reader's heap buffer (if any, wiped first). */
FIO_IFUNC void fio___tls13_pem_reader_destroy(fio___tls13_pem_reader_s *r) {
  if (r->heap) {
    fio_secure_zero(r->heap, r->heap_len);
    FIO_MEM_FREE(r->heap, r->heap_len);
  }
  r->heap = NULL;
  r->heap_len = 0;
}

/** Returns 1 if a decoded PEM block is a non-empty CERTIFICATE. */
FIO_IFUNC int fio___tls13_pem_is_cert(const fio_pem_s *blk) {
  return blk->der_len && blk->label_len == 11 &&
         !FIO_MEMCMP(blk->label, "CERTIFICATE", 11);
}

/**
 * Loads every CERTIFICATE block of a PEM file into ctx->cert_chain.
 *
 * Returns the number of certificates loaded, or -1 if any block can't be
 * decoded or stored (a partial chain is never reported as success; the
 * caller frees the chain on failure).
 */
FIO_SFUNC int fio___tls13_load_cert_pem_chain(fio___tls13_context_s *ctx,
                                              const char *pem_data,
                                              size_t pem_len) {
  int loaded = 0, rc;
  fio___tls13_pem_reader_s r;
  r.pos = pem_data;
  r.remaining = pem_len;
  r.heap = NULL;
  r.heap_len = 0;
  fio_pem_s blk;

  while ((rc = fio___tls13_pem_read(&r, &blk)) > 0) {
    if (!fio___tls13_pem_is_cert(&blk)) {
      /* e.g., a private key in a combined cert+key file */
      fio_secure_zero((void *)blk.der, blk.der_len);
      continue;
    }
    uint8_t *der_buf = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, blk.der_len, 0);
    if (!der_buf) {
      FIO_LOG_ERROR("TLS 1.3: failed to allocate certificate DER buffer");
      loaded = -1;
      break;
    }
    FIO_LEAK_COUNTER_ON_ALLOC(fio___tls13_der_buf);
    FIO_MEMCPY(der_buf, blk.der, blk.der_len);
    if (fio___tls13_context_cert_chain_add(ctx, der_buf, blk.der_len)) {
      FIO_LOG_ERROR("TLS 1.3: failed to store certificate chain entry");
      loaded = -1;
      break;
    }
    ++loaded;
  }
  if (rc < 0) {
    FIO_LOG_ERROR("TLS 1.3: undecodable PEM block in certificate file "
                  "(after %d certificate(s))",
                  loaded);
    loaded = -1;
    /* the failed block may have been partially decoded (maybe a key) */
    fio_secure_zero(r.stack, sizeof(r.stack));
  }
  fio___tls13_pem_reader_destroy(&r);
  return loaded;
}

/** Returns 1 if two big-endian unsigned integers are equal (leading zeros
 * ignored). Public values only (not constant time). */
FIO_IFUNC int fio___tls13_be_uint_eq(const uint8_t *a,
                                     size_t a_len,
                                     const uint8_t *b,
                                     size_t b_len) {
  while (a_len && !a[0])
    ++a, --a_len;
  while (b_len && !b[0])
    ++b, --b_len;
  return a_len == b_len && (!a_len || !FIO_MEMCMP(a, b, a_len));
}

/**
 * Returns 1 if `pt` (a SEC 1 point with `n` byte coordinates: uncompressed
 * `04 || x || y` (1 + 2n bytes) or compressed `02/03 || x` (1 + n bytes))
 * encodes the same point as `pub` (uncompressed, on the curve). The
 * compressed form is compared as x plus the parity of y, which identifies a
 * curve point uniquely. Constant time for a given `len`.
 */
FIO_SFUNC int fio___tls13_ec_point_eq(const uint8_t *pub,
                                      size_t n,
                                      const uint8_t *pt,
                                      size_t len) {
  int r = 0;
  if (len == 1 + 2 * n) {
    r = fio_ct_is_eq(pub, pt, len);
  } else if (len == 1 + n) {
    uint8_t c[1 + 48];
    c[0] = (uint8_t)(0x02 | (pub[2 * n] & 1));
    FIO_MEMCPY(c + 1, pub + 1, n);
    r = fio_ct_is_eq(c, pt, len);
    fio_secure_zero(c, sizeof(c));
  }
  return r;
}

/** `fio___tls13_ec_point_eq` for P-256 (`pub`: 65 bytes uncompressed). */
FIO_SFUNC int fio___tls13_p256_point_eq(const uint8_t pub[65],
                                        const uint8_t *pt,
                                        size_t len) {
  return fio___tls13_ec_point_eq(pub, 32, pt, len);
}

/** `fio___tls13_ec_point_eq` for P-384 (`pub`: 97 bytes uncompressed). */
FIO_SFUNC int fio___tls13_p384_point_eq(const uint8_t pub[97],
                                        const uint8_t *pt,
                                        size_t len) {
  return fio___tls13_ec_point_eq(pub, 48, pt, len);
}

/**
 * Checks that the private key matches the leaf certificate's public key
 * (same purpose as OpenSSL's `X509_check_private_key`).
 *
 * The public key is derived from the private key (P-256, P-384, Ed25519) or
 * taken
 * from its public components (RSA `n` and `e`) and compared with the leaf's
 * SubjectPublicKeyInfo. Context-build time only.
 *
 * Returns 0 on match, -1 on mismatch / unsupported / unparsable leaf.
 */
FIO_SFUNC int fio___tls13_key_matches_leaf(const fio___tls13_context_s *ctx,
                                           const fio_pem_private_key_s *k) {
  fio_x509_cert_s leaf;
  int r = -1;
  if (!ctx->cert_chain_count ||
      fio_x509_parse(&leaf, ctx->cert_chain[0].buf, ctx->cert_chain[0].len))
    return r;
  switch (k->type) {
  case FIO_PEM_KEY_ECDSA_P256:
#if defined(H___FIO_P256___H)
    if (leaf.key_algo == FIO_X509_KEY_ECDSA_P256) {
      /* the leaf point may be uncompressed (65) or compressed (33 bytes) */
      uint8_t pub[65];
      if (!fio_p256_public_key(pub, k->ecdsa_p256.private_key))
        r = 0 - !fio___tls13_p256_point_eq(pub,
                                           leaf.pubkey.ecdsa.point.buf,
                                           leaf.pubkey.ecdsa.point.len);
      fio_secure_zero(pub, sizeof(pub));
    }
#endif
    break;
  case FIO_PEM_KEY_ECDSA_P384:
#if defined(H___FIO_P384___H)
    if (leaf.key_algo == FIO_X509_KEY_ECDSA_P384) {
      /* the leaf point may be uncompressed (97) or compressed (49 bytes) */
      uint8_t pub[97];
      if (!fio_p384_public_key(pub, k->ecdsa_p384.private_key))
        r = 0 - !fio___tls13_p384_point_eq(pub,
                                           leaf.pubkey.ecdsa.point.buf,
                                           leaf.pubkey.ecdsa.point.len);
      fio_secure_zero(pub, sizeof(pub));
    }
#endif
    break;
  case FIO_PEM_KEY_ED25519:
#if defined(H___FIO_ED25519___H)
    if (leaf.key_algo == FIO_X509_KEY_ED25519 &&
        leaf.pubkey.ed25519.key.len == 32) {
      uint8_t pub[32];
      fio_ed25519_public_key(pub, k->ed25519.private_key);
      r = 0 - !fio_ct_is_eq(pub, leaf.pubkey.ed25519.key.buf, 32);
      if (!r && k->ed25519.has_public_key &&
          !fio_ct_is_eq(pub, k->ed25519.public_key, 32))
        r = -1; /* the key file's embedded public key disagrees */
      fio_secure_zero(pub, sizeof(pub));
    }
#endif
    break;
  case FIO_PEM_KEY_RSA:
    if (leaf.key_algo == FIO_X509_KEY_RSA &&
        fio___tls13_be_uint_eq(k->rsa.n,
                               k->rsa.n_len,
                               leaf.pubkey.rsa.n.buf,
                               leaf.pubkey.rsa.n.len) &&
        fio___tls13_be_uint_eq(k->rsa.e,
                               k->rsa.e_len,
                               leaf.pubkey.rsa.e.buf,
                               leaf.pubkey.rsa.e.len))
      r = 0;
    break;
  default: break;
  }
  return r;
}
#endif /* H___FIO_PEM___H && H___FIO_X509___H */

/** Callback for iterating certificates in fio_io_tls_s */
FIO_SFUNC int fio___tls13_each_cert(struct fio_io_tls_each_s *e,
                                    const char *server_name,
                                    const char *public_cert_file,
                                    const char *private_key_file,
                                    const char *pk_password) {
  fio___tls13_context_s *ctx = (fio___tls13_context_s *)e->udata;

  /* Load certificate and key from PEM files; any failure fails the context
   * build (fail closed, same as the OpenSSL backend). */
  if (public_cert_file && private_key_file) {
#if defined(H___FIO_PEM___H) && defined(H___FIO_X509___H)
    FIO_LOG_DEBUG2("TLS 1.3: loading certificate from %s", public_cert_file);
    FIO_LOG_DEBUG2("TLS 1.3: loading private key from %s", private_key_file);

    /* Read certificate file */
    char *cert_pem = fio_bstr_readfile(NULL, public_cert_file, 0, 0);
    if (!cert_pem) {
      FIO_LOG_ERROR("TLS 1.3: failed to read certificate file: %s",
                    public_cert_file);
      goto pem_failed;
    }

    /* Read private key file */
    char *key_pem = fio_bstr_readfile(NULL, private_key_file, 0, 0);
    if (!key_pem) {
      FIO_LOG_ERROR("TLS 1.3: failed to read private key file: %s",
                    private_key_file);
      fio___tls13_bstr_free_secret(cert_pem); /* may hold a combined key */
      goto pem_failed;
    }

    /* Load every CERTIFICATE block so servers send the full chain. */
    size_t cert_pem_len = fio_bstr_len(cert_pem);
    int certs_loaded =
        fio___tls13_load_cert_pem_chain(ctx, cert_pem, cert_pem_len);
    fio___tls13_bstr_free_secret(cert_pem); /* may hold a combined key */

    if (certs_loaded <= 0) {
      FIO_LOG_ERROR("TLS 1.3: failed to load certificate chain from PEM");
      fio___tls13_context_cert_chain_free(ctx); /* never keep a partial chain */
      fio___tls13_bstr_free_secret(key_pem);
      goto pem_failed;
    }

    /* Parse private key */
    fio_pem_private_key_s pkey;
    size_t key_pem_len = fio_bstr_len(key_pem);
    if (fio_pem_parse_private_key(&pkey, key_pem, key_pem_len) != 0) {
      FIO_LOG_ERROR("TLS 1.3: failed to parse private key PEM");
      fio_pem_private_key_clear(&pkey);
      fio___tls13_context_cert_chain_free(ctx);
      fio___tls13_bstr_free_secret(key_pem);
      goto pem_failed;
    }
    fio___tls13_bstr_free_secret(key_pem);

    /* The key must match the leaf certificate (fail closed, OpenSSL parity) */
    if (fio___tls13_key_matches_leaf(ctx, &pkey)) {
      FIO_LOG_ERROR("TLS 1.3: private key does not match the certificate "
                    "(or unsupported key / certificate)");
      fio_pem_private_key_clear(&pkey);
      fio___tls13_context_cert_chain_free(ctx);
      goto pem_failed;
    }

    /* Copy private key based on type */
    switch (pkey.type) {
    case FIO_PEM_KEY_ECDSA_P256:
#if defined(H___FIO_P256___H)
      /* Derive the public key; an embedded one must match it (one-time). */
      if (fio_p256_public_key(ctx->public_key, pkey.ecdsa_p256.private_key) ||
          (pkey.ecdsa_p256.has_public_key &&
           !fio___tls13_p256_point_eq(ctx->public_key,
                                      pkey.ecdsa_p256.public_key,
                                      pkey.ecdsa_p256.public_key_len))) {
        FIO_LOG_ERROR("TLS 1.3: invalid P-256 private key (out of range or "
                      "public key mismatch)");
        fio_secure_zero(ctx->public_key, 65);
        fio_pem_private_key_clear(&pkey);
        fio___tls13_context_cert_chain_free(ctx);
        goto pem_failed;
      }
      fio_memcpy32(ctx->private_key, pkey.ecdsa_p256.private_key);
#else
      FIO_LOG_ERROR("TLS 1.3: P-256 module required for key loading");
      fio_pem_private_key_clear(&pkey);
      fio___tls13_context_cert_chain_free(ctx);
      goto pem_failed;
#endif
      ctx->private_key_len = 32;
      ctx->signature_algo = FIO_TLS13_SIGNATURE_ECDSA_SECP256R1_SHA256;
      FIO_LOG_DEBUG2("TLS 1.3: loaded P-256 private key from PEM");
      break;

    case FIO_PEM_KEY_ECDSA_P384: {
#if defined(H___FIO_P384___H)
      /* Derive the public key; an embedded one must match it (one-time). */
      uint8_t pub[97];
      int bad = fio_p384_public_key(pub, pkey.ecdsa_p384.private_key) ||
                (pkey.ecdsa_p384.has_public_key &&
                 !fio___tls13_p384_point_eq(pub,
                                            pkey.ecdsa_p384.public_key,
                                            pkey.ecdsa_p384.public_key_len));
      fio_secure_zero(pub, sizeof(pub));
      if (bad) {
        FIO_LOG_ERROR("TLS 1.3: invalid P-384 private key (out of range or "
                      "public key mismatch)");
        fio_pem_private_key_clear(&pkey);
        fio___tls13_context_cert_chain_free(ctx);
        goto pem_failed;
      }
      FIO_MEMCPY(ctx->private_key, pkey.ecdsa_p384.private_key, 48);
      /* `public_key` is P-256 only (the P-384 key signs without it) */
      FIO_MEMSET(ctx->public_key, 0, sizeof(ctx->public_key));
#else
      FIO_LOG_ERROR("TLS 1.3: P-384 module required for key loading");
      fio_pem_private_key_clear(&pkey);
      fio___tls13_context_cert_chain_free(ctx);
      goto pem_failed;
#endif
      ctx->private_key_len = 48;
      ctx->signature_algo = FIO_TLS13_SIGNATURE_ECDSA_SECP384R1_SHA384;
      FIO_LOG_DEBUG2("TLS 1.3: loaded P-384 private key from PEM");
      break;
    }

    case FIO_PEM_KEY_ED25519:
      fio_memcpy32(ctx->private_key, pkey.ed25519.private_key);
      ctx->private_key_len = 32;
      ctx->signature_algo = FIO_TLS13_SIGNATURE_ED25519;
      FIO_LOG_DEBUG2("TLS 1.3: loaded Ed25519 private key from PEM");
      break;

    case FIO_PEM_KEY_RSA: {
#if defined(H___FIO_RSA___H)
      /* Encode the RSA private key in the TLS 1.3 key format:
       * [len:4][value] for n, d, e, p, q, dP, dQ, qInv (absent: length 0).
       * With e and the CRT values, signing uses CRT with blinding and checks
       * each signature (faults never emit one); n and d alone can't. */
      const uint8_t *rsa_v[8] = {pkey.rsa.n,
                                 pkey.rsa.d,
                                 pkey.rsa.e,
                                 pkey.rsa.p,
                                 pkey.rsa.q,
                                 pkey.rsa.dP,
                                 pkey.rsa.dQ,
                                 pkey.rsa.qInv};
      const size_t rsa_l[8] = {pkey.rsa.n_len,
                               pkey.rsa.d_len,
                               pkey.rsa.e_len,
                               pkey.rsa.p_len,
                               pkey.rsa.q_len,
                               pkey.rsa.dP_len,
                               pkey.rsa.dQ_len,
                               pkey.rsa.qInv_len};
      size_t encoded_len = 0;
      int rsa_bad = !rsa_l[0] || !rsa_l[1];
      for (size_t i = 0; i < 8; ++i) {
        rsa_bad |= rsa_l[i] > FIO_RSA_MAX_BYTES;
        encoded_len += 4 + rsa_l[i];
      }
      if (rsa_bad) {
        FIO_LOG_ERROR("TLS 1.3: RSA private key has invalid lengths");
        fio_pem_private_key_clear(&pkey);
        fio___tls13_context_cert_chain_free(ctx);
        goto pem_failed;
      }
      uint8_t *encoded = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, encoded_len, 0);
      if (encoded)
        FIO_LEAK_COUNTER_ON_ALLOC(fio___tls13_der_buf);
      if (!encoded) {
        FIO_LOG_ERROR("TLS 1.3: failed to allocate RSA private key buffer");
        fio_pem_private_key_clear(&pkey);
        fio___tls13_context_cert_chain_free(ctx);
        goto pem_failed;
      }
      for (size_t i = 0, o = 0; i < 8; ++i) {
        fio_u2buf32_be(encoded + o, (uint32_t)rsa_l[i]);
        o += 4;
        if (rsa_l[i])
          FIO_MEMCPY(encoded + o, rsa_v[i], rsa_l[i]);
        o += rsa_l[i];
      }

      ctx->private_key_ext.buf = encoded;
      ctx->private_key_ext.len = encoded_len;
      ctx->private_key_len = 0;
      ctx->signature_algo = FIO_TLS13_SIGNATURE_RSA_PSS_RSAE_SHA256;
      FIO_LOG_DEBUG2("TLS 1.3: loaded RSA private key from PEM (n=%zu)",
                     pkey.rsa.n_len);
      break;
#else
      FIO_LOG_ERROR(
          "TLS 1.3: RSA private key loaded but RSA module not available");
      fio_pem_private_key_clear(&pkey);
      fio___tls13_context_cert_chain_free(ctx);
      goto pem_failed;
#endif
    }

    default:
      FIO_LOG_ERROR("TLS 1.3: unsupported private key type");
      fio_pem_private_key_clear(&pkey);
      fio___tls13_context_cert_chain_free(ctx);
      goto pem_failed;
    }

    fio_pem_private_key_clear(&pkey);
    FIO_LOG_DEBUG2("TLS 1.3: loaded %d certificate(s) from PEM chain",
                   certs_loaded);
    (void)pk_password;
    (void)server_name;
    return 0;

  pem_failed:
    FIO_LOG_ERROR("TLS 1.3: couldn't load certificate / private key files: "
                  "%s & %s",
                  public_cert_file,
                  private_key_file);
    return -1;
#else
    FIO_LOG_ERROR("TLS 1.3: PEM and X509 modules required to load: %s",
                  public_cert_file);
    return -1;
#endif /* H___FIO_PEM___H && H___FIO_X509___H */
  }

  /* Clients: a name without files only names the server (SNI); a generated
   * certificate would only fail the server's verification, so present none. */
  if (ctx->is_client)
    return 0;

#if defined(H___FIO_X509___H)
  /* Names added without files: generate a self-signed certificate. */
  if (fio___tls13_make_self_signed(ctx, server_name) != 0) {
    FIO_LOG_ERROR("TLS 1.3: failed to create self-signed certificate");
    return -1;
  }
#else
  FIO_LOG_ERROR(
      "TLS 1.3: X509 module not available for certificate generation");
  return -1;
#endif

  (void)pk_password;
  return 0;
}

/** Loads one of several server certificates and indexes it by name (SNI). */
FIO_SFUNC int fio___tls13_each_sni_cert(struct fio_io_tls_each_s *e,
                                        const char *server_name,
                                        const char *public_cert_file,
                                        const char *private_key_file,
                                        const char *pk_password) {
  fio___tls13_context_s *ctx = (fio___tls13_context_s *)e->udata;
  size_t *pos = (size_t *)e->udata2;
  if (*pos >= ctx->creds_count)
    return -1; /* certificates added while building? never overflow */
  /* load into the context's (empty) default fields, then move to its slot */
  if (fio___tls13_each_cert(e,
                            server_name,
                            public_cert_file,
                            private_key_file,
                            pk_password))
    return -1;
  FIO___TLS13_CRED_MOVE(ctx->creds + *pos, ctx);
  ctx->cert_der = (fio_ubuf_info_s){0};
  if (fio___io_tls_sni_add(&ctx->sni, server_name, *pos))
    return -1;
  ++*pos;
  return 0;
}

#if defined(H___FIO_X509___H) && defined(H___FIO_PEM___H)

/** Internal: append one owned DER buffer to a trust store array pair.
 *
 * Takes ownership of der_buf (allocated with FIO_MEM_REALLOC, exactly
 * der_len bytes).  alloc_len MUST equal the exact allocation size passed to
 * FIO_MEM_REALLOC — the free path uses it to call FIO_MEM_FREE correctly.
 * When der_buf was allocated exactly der_len bytes, alloc_len == der_len.
 *
 * Returns 0 on success, -1 on allocation failure (der_buf is freed on error).
 */
FIO_SFUNC int fio___tls13_trust_array_push(uint8_t ***bufs_p,
                                           size_t **lens_p,
                                           size_t *count_p,
                                           uint8_t *der_buf,
                                           size_t der_len) {
  size_t old_count = *count_p;
  size_t new_count = old_count + 1;
  uint8_t **new_bufs =
      (uint8_t **)FIO_MEM_REALLOC(*bufs_p,
                                  old_count * sizeof(uint8_t *),
                                  new_count * sizeof(uint8_t *),
                                  old_count * sizeof(uint8_t *));
  size_t *new_lens = (size_t *)FIO_MEM_REALLOC(*lens_p,
                                               old_count * sizeof(size_t),
                                               new_count * sizeof(size_t),
                                               old_count * sizeof(size_t));
  if (!new_bufs || !new_lens) {
    FIO_LOG_ERROR("TLS 1.3: failed to allocate trust store arrays");
    FIO_LEAK_COUNTER_ON_FREE(fio___tls13_der_buf);
    FIO_MEM_FREE(der_buf, der_len);
    if (new_bufs)
      *bufs_p = new_bufs;
    if (new_lens)
      *lens_p = new_lens;
    return -1;
  }
  *bufs_p = new_bufs;
  *lens_p = new_lens;
  (*bufs_p)[old_count] = der_buf;
  (*lens_p)[old_count] = der_len; /* alloc size == DER content size */
  *count_p = new_count;
  return 0;
}

/** Internal: append one owned DER buffer to the context's trust store arrays.
 * Wrapper around fio___tls13_trust_array_push that also updates
 * trust_store.roots/root_lens/root_count to point at the (possibly
 * reallocated) arrays. */
FIO_SFUNC int fio___tls13_add_der_to_trust(fio___tls13_context_s *ctx,
                                           uint8_t *der_buf,
                                           size_t der_len) {
  if (fio___tls13_trust_array_push(&ctx->trust_der_bufs,
                                   &ctx->trust_der_lens,
                                   &ctx->trust_der_count,
                                   der_buf,
                                   der_len) != 0)
    return -1;
  /* Update trust_store to point to the (possibly reallocated) arrays */
  ctx->trust_store.roots = (const uint8_t **)ctx->trust_der_bufs;
  ctx->trust_store.root_lens = ctx->trust_der_lens;
  ctx->trust_store.root_count = ctx->trust_der_count;
  return 0;
}

/** Internal: load all CERTIFICATE blocks from a PEM bundle into a trust
 * store array.  Iterates through all certs in the bundle (CA bundles contain
 * hundreds).
 *
 * Undecodable PEM blocks are logged (FIO_LOG_WARNING) and skipped: a missing
 * root can only make verification fail closed, and one malformed entry must
 * not disable a whole system CA bundle.  Blocks larger than the 16 KiB stack
 * buffer are decoded through a heap buffer, so they load normally.
 *
 * Each certificate is copied into an exactly-sized allocation, so
 * FIO_MEM_FREE(bufs[i], lens[i]) is always correct.
 *
 * Returns the number of certificates loaded (>= 0), or -1 on allocation
 * failure before any certificate was loaded. */
FIO_SFUNC int fio___tls13_load_pem_bundle_into(uint8_t ***bufs_p,
                                               size_t **lens_p,
                                               size_t *count_p,
                                               const char *pem_data,
                                               size_t pem_len) {
  int loaded = 0, rc;
  fio___tls13_pem_reader_s r;
  r.pos = pem_data;
  r.remaining = pem_len;
  r.heap = NULL;
  r.heap_len = 0;
  fio_pem_s blk;

  while ((rc = fio___tls13_pem_read(&r, &blk)) != 0) {
    if (rc < 0) {
      FIO_LOG_WARNING("TLS 1.3: skipping undecodable PEM block in CA bundle "
                      "(after %d certificate(s))",
                      loaded);
      continue;
    }
    /* Only accept CERTIFICATE blocks (skip PRIVATE KEY etc.) */
    if (!fio___tls13_pem_is_cert(&blk))
      continue;
    uint8_t *der_buf = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, blk.der_len, 0);
    if (!der_buf) {
      FIO_LOG_ERROR("TLS 1.3: failed to allocate DER buffer for PEM bundle");
      if (!loaded)
        loaded = -1;
      break;
    }
    FIO_LEAK_COUNTER_ON_ALLOC(fio___tls13_der_buf);
    FIO_MEMCPY(der_buf, blk.der, blk.der_len);
    if (fio___tls13_trust_array_push(bufs_p,
                                     lens_p,
                                     count_p,
                                     der_buf,
                                     blk.der_len) != 0) {
      if (!loaded)
        loaded = -1;
      break; /* der_buf freed by helper */
    }
    ++loaded;
  }
  fio___tls13_pem_reader_destroy(&r);
  return loaded;
}

/** Internal: load all CERTIFICATE blocks from a PEM bundle into a context's
 * trust store.  Wrapper that updates trust_store pointers after loading. */
FIO_SFUNC int fio___tls13_load_pem_bundle(fio___tls13_context_s *ctx,
                                          const char *pem_data,
                                          size_t pem_len) {
  size_t old_count = ctx->trust_der_count;
  int loaded = fio___tls13_load_pem_bundle_into(&ctx->trust_der_bufs,
                                                &ctx->trust_der_lens,
                                                &ctx->trust_der_count,
                                                pem_data,
                                                pem_len);
  /* The per-context arrays are owned once allocated, even after a failure,
   * so the context's free paths release them. */
  if (ctx->trust_der_bufs || ctx->trust_der_lens)
    ctx->trust_der_owned = 1;
  if (loaded > 0 && ctx->trust_der_count > old_count) {
    /* Update trust_store to reflect the newly added entries */
    ctx->trust_store.roots = (const uint8_t **)ctx->trust_der_bufs;
    ctx->trust_store.root_lens = ctx->trust_der_lens;
    ctx->trust_store.root_count = ctx->trust_der_count;
  }
  return loaded;
}

/** Internal: get (or lazily load) the global system CA trust store singleton.
 *
 * Problem 2 fix: the system CA bundle is loaded ONCE into a process-wide
 * singleton and shared (read-only) across all TLS contexts.  This prevents
 * loading ~128 certs from disk and allocating ~192 KB of DER per client
 * connection.  After this function returns successfully,
 * fio___tls13_sys_trust.store is valid for the lifetime of the process.
 *
 * Thread safety: protected by fio___tls13_sys_trust_lock.
 * Returns the number of CA certs loaded (>= 1) on success, 0 if not found. */
FIO_SFUNC int fio___tls13_get_system_trust(void) {
  fio_lock(&fio___tls13_sys_trust_lock);
  if (fio___tls13_sys_trust.loaded) {
    int n = (int)fio___tls13_sys_trust.count;
    fio_unlock(&fio___tls13_sys_trust_lock);
    return n;
  }

  int loaded = 0;

#if defined(_WIN32)
  /* Windows: enumerate the "ROOT" system certificate store via CryptoAPI */
  HCERTSTORE hStore = CertOpenSystemStoreA(0, "ROOT");
  if (!hStore) {
    FIO_LOG_ERROR("TLS 1.3: failed to open Windows ROOT certificate store");
    fio_unlock(&fio___tls13_sys_trust_lock);
    return 0;
  }
  PCCERT_CONTEXT pCert = NULL;
  while ((pCert = CertEnumCertificatesInStore(hStore, pCert)) != NULL) {
    if (pCert->dwCertEncodingType != X509_ASN_ENCODING ||
        pCert->cbCertEncoded == 0)
      continue;
    size_t der_len = (size_t)pCert->cbCertEncoded;
    uint8_t *der_buf = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, der_len, 0);
    if (!der_buf)
      continue;
    FIO_LEAK_COUNTER_ON_ALLOC(fio___tls13_der_buf);
    FIO_MEMCPY(der_buf, pCert->pbCertEncoded, der_len);
    if (fio___tls13_trust_array_push(&fio___tls13_sys_trust.bufs,
                                     &fio___tls13_sys_trust.lens,
                                     &fio___tls13_sys_trust.count,
                                     der_buf,
                                     der_len) == 0)
      ++loaded;
  }
  CertCloseStore(hStore, 0);
  if (loaded > 0)
    FIO_LOG_DEBUG2("TLS 1.3: loaded %d CA certs from Windows ROOT store",
                   loaded);

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
    loaded = fio___tls13_load_pem_bundle_into(&fio___tls13_sys_trust.bufs,
                                              &fio___tls13_sys_trust.lens,
                                              &fio___tls13_sys_trust.count,
                                              pem,
                                              pem_len);
    fio_bstr_free(pem);

    if (loaded > 0) {
      FIO_LOG_DEBUG2("TLS 1.3: loaded %d CA certs into global system store "
                     "from %s",
                     loaded,
                     ca_paths[i]);
      break;
    }
  }
#endif /* _WIN32 */

  if (loaded > 0) {
    /* Populate the trust_store view into the global arrays */
    fio___tls13_sys_trust.store.roots =
        (const uint8_t **)fio___tls13_sys_trust.bufs;
    fio___tls13_sys_trust.store.root_lens = fio___tls13_sys_trust.lens;
    fio___tls13_sys_trust.store.root_count = fio___tls13_sys_trust.count;
    fio___tls13_sys_trust.loaded = 1;
    /* Cleanup is registered once at startup via FIO_CONSTRUCTOR — not here. */
  }

  fio_unlock(&fio___tls13_sys_trust_lock);
  return loaded;
}

/** Callback for iterating trust certificates in fio_io_tls_s.
 *
 * Loads each PEM trust certificate into the context's trust store.
 * Called with public_cert_file == NULL when trust_sys is set — uses the
 * global system CA trust store singleton (Problem 2 fix). */
FIO_SFUNC int fio___tls13_each_trust(struct fio_io_tls_each_s *e,
                                     const char *public_cert_file) {
  fio___tls13_context_s *ctx = (fio___tls13_context_s *)e->udata;

  if (!public_cert_file) {
    /* System trust store requested: use global singleton (loaded once). */
    int loaded = fio___tls13_get_system_trust();
    if (loaded <= 0) {
      FIO_LOG_ERROR("TLS 1.3: system trust store not found or empty — "
                    "cannot verify server certificates. Add CA certificates "
                    "explicitly via fio_io_tls_trust_add().");
      return -1;
    }
    /* Point context trust_store at the global singleton (non-owning ref).
     * trust_der_owned stays 0 — context must NOT free these buffers. */
    ctx->trust_store = fio___tls13_sys_trust.store;
    return 0;
  }

  /* Read PEM file (user-configured trust cert — owned by this context) */
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

  ctx->trust_der_owned = 1;
  FIO_LOG_DEBUG2("TLS 1.3: loaded %d trust certificate(s) from %s",
                 loaded,
                 public_cert_file);
  return 0;
}
#endif /* H___FIO_X509___H && H___FIO_PEM___H */

/**
 * Loads the configured certificates (fail closed). A single certificate is
 * always used; with several, each is loaded once and indexed by name (SNI,
 * servers only) and the default one is moved into the context's own fields
 * (servers present it by default, clients when the server requests one).
 */
FIO_SFUNC int fio___tls13_context_load_certs(fio___tls13_context_s *ctx,
                                             fio_io_tls_s *tls) {
  if (fio_io_tls_cert_count(tls) == 1) {
    if (fio_io_tls_each(tls, .udata = ctx, .each_cert = fio___tls13_each_cert))
      goto failed;
    return 0;
  }
  if (!fio_io_tls_cert_count(tls))
    return 0;
  size_t pos = 0;
  ctx->creds_count = fio_io_tls_cert_count(tls);
  ctx->creds = (fio___tls13_cred_s *)
      FIO_MEM_REALLOC(NULL, 0, ctx->creds_count * sizeof(*ctx->creds), 0);
  if (!ctx->creds) {
    ctx->creds_count = 0;
    FIO_LOG_ERROR("TLS 1.3: memory allocation failed for certificates");
    return -1;
  }
  FIO_MEMSET(ctx->creds, 0, ctx->creds_count * sizeof(*ctx->creds));
  if (fio_io_tls_each(tls,
                      .udata = ctx,
                      .udata2 = &pos,
                      .each_cert = fio___tls13_each_sni_cert) ||
      pos != ctx->creds_count)
    goto failed;
  FIO___TLS13_CRED_MOVE(ctx, ctx->creds + ctx->sni.dflt);
  /* a client's default may be a name-only entry (no certificate) */
  ctx->cert_der =
      ctx->cert_chain_count ? ctx->cert_chain[0] : (fio_ubuf_info_s){0};
  return 0;
failed:
  FIO_LOG_ERROR("TLS 1.3: failed to configure certificates");
  return -1;
}

/** Sets `ctx->flight_size` from its largest certificate chain (the default
 * one and every SNI entry). */
FIO_SFUNC void fio___tls13_context_flight_size(fio___tls13_context_s *ctx) {
  ctx->flight_size =
      fio_tls13_flight_size(ctx->cert_chain, ctx->cert_chain_count);
  for (size_t i = 0; i < ctx->creds_count; ++i) {
    size_t n = fio_tls13_flight_size(ctx->creds[i].cert_chain,
                                     ctx->creds[i].cert_chain_count);
    if (n > ctx->flight_size)
      ctx->flight_size = n;
  }
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
  ctx->ref = 1;
  ctx->is_client = is_client;

  /* For client, extract server name from TLS object for SNI */
  if (is_client) {
    /* Prefer the explicit SNI host name (set from the URL by fio_io_connect).
     */
    fio_buf_info_s sni = fio_keystr_buf(&tls->sni);
    if (sni.buf && sni.len && sni.len < sizeof(ctx->server_name)) {
      FIO_MEMCPY(ctx->server_name, sni.buf, sni.len);
      ctx->server_name[sni.len] = '\0';
      FIO_LOG_DEBUG2("TLS 1.3: extracted SNI hostname: %s", ctx->server_name);
    } else {
      /* Fall back to the first certificate name (legacy behavior). */
      FIO_IMAP_EACH(fio___io_tls_cert_map, &tls->cert, i) {
        fio_buf_info_s nm = fio_keystr_buf(&tls->cert.ary[i].nm);
        if (nm.buf && nm.len > 0 && nm.len < sizeof(ctx->server_name)) {
          FIO_MEMCPY(ctx->server_name, nm.buf, nm.len);
          ctx->server_name[nm.len] = '\0';
          FIO_LOG_DEBUG2("TLS 1.3: extracted SNI hostname: %s",
                         ctx->server_name);
          break;
        }
      }
    }
#if defined(H___FIO_X509___H) && defined(H___FIO_PEM___H)
    /* Build trust store from configured trust certificates.
     *
     * Trust semantics (fio_io_tls_trust_add always refers to PEER
     * verification):
     * - trust certificates listed: verify the peer (server) against them.
     * - fio_io_tls_trust_add(tls, NULL): verify against the system store.
     * - empty trust list: NO peer verification (a SECURITY warning is
     *   logged and verification is skipped when the handshake starts).
     *
     * The system CA store is loaded ONCE into a global singleton
     * (fio___tls13_sys_trust) and shared across all contexts.  Per-context
     * trust_der_bufs/lens are only allocated for user-configured certs
     * (trust_der_owned=1).  System trust is a non-owning reference
     * (trust_der_owned=0). */
    if (fio_io_tls_trust_count(tls) || tls->trust_sys) {
      /* Explicit trust config: load via iterator.
       * fio___tls13_each_trust handles both file certs (owned, per-context)
       * and trust_sys → global singleton (non-owned). */
      if (fio_io_tls_each(tls,
                          .udata = ctx,
                          .each_trust = fio___tls13_each_trust)) {
        FIO_LOG_ERROR("TLS 1.3: failed to load trust certificates");
        goto error;
      }
    }
#endif /* H___FIO_X509___H && H___FIO_PEM___H */
    /* Client certificate (mTLS), sent when the server requests one */
    if (fio___tls13_context_load_certs(ctx, tls))
      goto error;
  } else { /* For server, load certificates */
    /* Check if certificates are configured */
    if (!fio_io_tls_cert_count(tls)) {
      /* No certificates configured - generate self-signed into context */
#if defined(H___FIO_X509___H)
      if (fio___tls13_make_self_signed(ctx, "localhost") != 0) {
        FIO_LOG_ERROR("TLS 1.3: failed to create self-signed certificate");
        goto error;
      }
#else
      FIO_LOG_ERROR("TLS 1.3: X509 module required for server certificates");
      goto error;
#endif
    } else if (fio___tls13_context_load_certs(ctx, tls)) {
      goto error;
    }

#if defined(H___FIO_X509___H) && defined(H___FIO_PEM___H)
    /* Load trust store for client certificate verification if configured.
     * Trust always refers to peer verification: for a server context, the
     * peer is the client.  An empty trust list means no client certificate
     * is requested or verified. */
    if (fio_io_tls_trust_count(tls) || tls->trust_sys) {
      if (fio_io_tls_each(tls,
                          .udata = ctx,
                          .each_trust = fio___tls13_each_trust)) {
        FIO_LOG_ERROR("TLS 1.3: failed to load client trust certificates");
        goto error;
      }
    } else if (tls->trust_sys) {
      int loaded = fio___tls13_get_system_trust();
      if (loaded <= 0) {
        FIO_LOG_ERROR("TLS 1.3: failed to load system trust store for client "
                      "certificate verification");
        goto error;
      }
      ctx->trust_store = fio___tls13_sys_trust.store;
    }
#endif /* H___FIO_X509___H && H___FIO_PEM___H */
  }

  /* Collect ALPN protocols: client offers / server supported list. */
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

  fio___tls13_context_flight_size(ctx);
  return ctx;

error:
  if (ctx) {
    fio___tls13_context_creds_free(ctx);
    fio___tls13_context_cert_chain_free(ctx);
    fio_secure_zero(ctx->private_key, sizeof(ctx->private_key));
    if (ctx->private_key_ext.buf) {
      fio_secure_zero(ctx->private_key_ext.buf, ctx->private_key_ext.len);
      FIO_LEAK_COUNTER_ON_FREE(fio___tls13_der_buf);
      FIO_MEM_FREE(ctx->private_key_ext.buf, ctx->private_key_ext.len);
      ctx->private_key_ext.buf = NULL;
      ctx->private_key_ext.len = 0;
    }
#if defined(H___FIO_X509___H) && defined(H___FIO_PEM___H)
    /* Only free DER buffers when this context owns them (trust_der_owned=1).
     * When trust_der_owned=0 the arrays are the global system trust singleton
     * and must NOT be freed here. */
    if (ctx->trust_der_owned) {
      for (size_t i = 0; i < ctx->trust_der_count; ++i) {
        FIO_LEAK_COUNTER_ON_FREE(fio___tls13_der_buf);
        FIO_MEM_FREE(ctx->trust_der_bufs[i], ctx->trust_der_lens[i]);
      }
      if (ctx->trust_der_bufs)
        FIO_MEM_FREE(ctx->trust_der_bufs,
                     ctx->trust_der_count * sizeof(uint8_t *));
      if (ctx->trust_der_lens)
        FIO_MEM_FREE(ctx->trust_der_lens,
                     ctx->trust_der_count * sizeof(size_t));
    }
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

/** Drops a context reference, destroying the context on the last one. */
FIO_SFUNC void fio___tls13_context_release(fio___tls13_context_s *ctx) {
  if (!ctx || fio_atomic_sub_fetch(&ctx->ref, 1))
    return;
  FIO_LEAK_COUNTER_ON_FREE(fio___tls13_context_s);
  fio___tls13_context_creds_free(ctx);
  fio___tls13_context_cert_chain_free(ctx);
  fio_secure_zero(ctx->private_key, sizeof(ctx->private_key));
  if (ctx->private_key_ext.buf) {
    fio_secure_zero(ctx->private_key_ext.buf, ctx->private_key_ext.len);
    FIO_LEAK_COUNTER_ON_FREE(fio___tls13_der_buf);
    FIO_MEM_FREE(ctx->private_key_ext.buf, ctx->private_key_ext.len);
    ctx->private_key_ext.buf = NULL;
    ctx->private_key_ext.len = 0;
  }
#if defined(H___FIO_X509___H) && defined(H___FIO_PEM___H)
  /* Only free DER buffers when this context owns them (trust_der_owned=1).
   * When trust_der_owned=0 the arrays are the global system trust singleton
   * and must NOT be freed here — they are freed at process exit. */
  if (ctx->trust_der_owned) {
    for (size_t i = 0; i < ctx->trust_der_count; ++i) {
      FIO_LEAK_COUNTER_ON_FREE(fio___tls13_der_buf);
      FIO_MEM_FREE(ctx->trust_der_bufs[i], ctx->trust_der_lens[i]);
    }
    if (ctx->trust_der_bufs)
      FIO_MEM_FREE(ctx->trust_der_bufs,
                   ctx->trust_der_count * sizeof(uint8_t *));
    if (ctx->trust_der_lens)
      FIO_MEM_FREE(ctx->trust_der_lens, ctx->trust_der_count * sizeof(size_t));
  }
#endif /* H___FIO_X509___H && H___FIO_PEM___H */
  if (ctx->tls)
    fio_io_tls_free(ctx->tls);
  FIO_MEM_FREE(ctx, sizeof(*ctx));
}

FIO_SFUNC void fio___tls13_free_context_task(void *tls_ctx, void *ignr_) {
  fio___tls13_context_release((fio___tls13_context_s *)tls_ctx);
  (void)ignr_;
}

/** Releases a context reference; live connections keep the context. */
FIO_SFUNC void fio___tls13_free_context(void *tls_ctx) {
  fio_io_defer(fio___tls13_free_context_task, tls_ctx, NULL);
}

/** Returns a new context reference (released by free_context). */
FIO_SFUNC void *fio___tls13_dup_context(void *tls_ctx) {
  if (tls_ctx)
    fio_atomic_add(&((fio___tls13_context_s *)tls_ctx)->ref, 1);
  return tls_ctx;
}

/* *****************************************************************************
TLS 1.3 Per-Connection Management
***************************************************************************** */

/** Allocate connection state with flexible array buffer */
FIO_SFUNC fio___tls13_connection_s *fio___tls13_connection_new(
    fio___tls13_context_s *ctx) {
  /* Single allocation for struct + all buffers (flex array member) */
  size_t alloc_size = sizeof(fio___tls13_connection_s) + FIO___TLS13_BUF_TOTAL;
  fio___tls13_connection_s *conn = NULL;
  /* the connection type is over-aligned (embedded vector unions) */
  FIO_MEM_REALLOC_ALIGNED(conn,
                          0,
                          alloc_size,
                          0,
                          _Alignof(fio___tls13_connection_s));
  if (!conn)
    return NULL;
  FIO_LEAK_COUNTER_ON_ALLOC(fio___tls13_connection_s);
  FIO_MEMSET(conn, 0, alloc_size);
  conn->out_buf = conn->out_inline;
  conn->out_buf_cap = sizeof(conn->out_inline);
  conn->is_client = ctx->is_client;
  conn->ctx = ctx;
  fio_atomic_add(&ctx->ref, 1); /* released by fio___tls13_connection_free */

  return conn;
}

/** Returns the wire-output queue to the inline buffer, freeing any larger
 * allocation. Any queued data is discarded. */
FIO_SFUNC void fio___tls13_out_shrink(fio___tls13_connection_s *conn) {
  if (conn->out_buf != conn->out_inline) {
    FIO_MEM_FREE(conn->out_buf, conn->out_buf_cap);
    conn->out_buf = conn->out_inline;
    conn->out_buf_cap = sizeof(conn->out_inline);
  }
}

/** Ensures `need` free bytes after the queued wire output, dropping the sent
 * prefix and, if still short, moving the queue to a large enough allocation.
 * Returns 0 on success, -1 on allocation failure (the queue is unchanged). */
FIO_SFUNC int fio___tls13_out_reserve(fio___tls13_connection_s *conn,
                                      size_t need) {
  if (conn->out_buf_cap - conn->out_buf_len >= need)
    return 0;
  if (conn->out_buf_sent) {
    conn->out_buf_len -= conn->out_buf_sent;
    FIO_MEMMOVE(conn->out_buf,
                conn->out_buf + conn->out_buf_sent,
                conn->out_buf_len);
    conn->out_buf_sent = 0;
    if (conn->out_buf_cap - conn->out_buf_len >= need)
      return 0;
  }
  size_t cap = conn->out_buf_len + need;
  uint8_t *buf = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, cap, 0);
  if (!buf)
    return -1;
  if (conn->out_buf_len)
    FIO_MEMCPY(buf, conn->out_buf, conn->out_buf_len);
  fio___tls13_out_shrink(conn);
  conn->out_buf = buf;
  conn->out_buf_cap = cap;
  return 0;
}

/** Free connection state */
FIO_SFUNC void fio___tls13_connection_free(fio___tls13_connection_s *conn) {
  if (!conn)
    return;
  fio___tls13_out_shrink(conn);

  /* Destroy TLS state */
  if (conn->is_client)
    fio_tls13_client_destroy(&conn->state.client);
  else
    fio_tls13_server_destroy(&conn->state.server);
  fio___tls13_context_release(conn->ctx);

  /* Single free for struct + all buffers (flex array member) */
  FIO_LEAK_COUNTER_ON_FREE(fio___tls13_connection_s);
  FIO_MEM_FREE_ALIGNED(conn, sizeof(*conn) + FIO___TLS13_BUF_TOTAL);
}

/** Drains serialized wire data, retaining any unsent suffix for retry. */
FIO_SFUNC ssize_t fio___tls13_out_flush(fio_socket_i fd,
                                        fio___tls13_connection_s *conn) {
  size_t total_flushed = 0;
  while (conn->out_buf_sent < conn->out_buf_len) {
    ssize_t written = fio_sock_write(fd,
                                     (char *)conn->out_buf + conn->out_buf_sent,
                                     conn->out_buf_len - conn->out_buf_sent);
    if (written <= 0)
      return total_flushed ? (ssize_t)total_flushed : -1;
    conn->out_buf_sent += (size_t)written;
    total_flushed += (size_t)written;
  }
  conn->out_buf_len = 0;
  conn->out_buf_sent = 0;
  fio___tls13_out_shrink(conn); /* a large handshake flight was sent */
  return (ssize_t)total_flushed;
}

/* *****************************************************************************
TLS 1.3 IO Functions - Start
***************************************************************************** */

/** Initializes a server connection's handshake state from its context. */
FIO_SFUNC void fio___tls13_server_setup(fio___tls13_connection_s *conn) {
  fio___tls13_context_s *ctx = conn->ctx;
  fio_tls13_server_init(&conn->state.server);

  /* Present the default certificate; with several certificates, the SNI
   * (ClientHello) selects one before it is sent. */
  FIO___TLS13_CRED_APPLY(&conn->state.server, ctx);
  if (ctx->creds_count)
    fio_tls13_server_on_hello(&conn->state.server, fio___tls13_on_hello, ctx);

  /* Set ALPN protocols if configured */
  if (ctx->alpn_protocols_len > 0) {
    fio_tls13_server_alpn_set(&conn->state.server, ctx->alpn_protocols);
  }

#if defined(H___FIO_X509___H) && defined(H___FIO_PEM___H)
  /* Require client certificates when a trust store is configured. */
  if (ctx->trust_store.root_count > 0) {
    fio_tls13_server_require_client_cert(&conn->state.server, 2);
    fio_tls13_server_set_trust_store(&conn->state.server, &ctx->trust_store);
  }
#endif /* H___FIO_X509___H && H___FIO_PEM___H */
}

/** Configures a client handshake from its context: SNI, ALPN, peer
 * verification and the client certificate (mTLS). */
FIO_SFUNC void fio___tls13_client_setup(fio___tls13_connection_s *conn) {
  fio___tls13_context_s *ctx = conn->ctx;
  /* Initialize the client with its SNI host name (if any) */
  const char *sni = ctx->server_name[0] ? ctx->server_name : NULL;
  fio_tls13_client_init(&conn->state.client, sni);
  if (ctx->alpn_protocols_len > 0)
    fio_tls13_client_alpn_set(&conn->state.client, ctx->alpn_protocols);

    /* Configure certificate verification.
     * The trust store is populated by fio___tls13_build_context only when
     * trust certificates were configured.  An empty trust list means no
     * peer (server) verification — skip verification and warn, matching
     * the OpenSSL backend's SSL_VERIFY_NONE behavior. */
#if defined(H___FIO_X509___H) && defined(H___FIO_PEM___H)
  if (ctx->trust_store.root_count > 0) {
    fio_tls13_client_set_trust_store(&conn->state.client, &ctx->trust_store);
  } else {
    FIO_LOG_SECURITY("no trusted TLS certificates listed for client, "
                     "skipping server certificate verification!");
    fio_tls13_client_skip_verification(&conn->state.client, 1);
  }
#else
  /* X509 module unavailable — cannot verify, skip (insecure). */
  fio_tls13_client_skip_verification(&conn->state.client, 1);
#endif /* H___FIO_X509___H && H___FIO_PEM___H */

  /* Client certificate (mTLS): the chain is sent only when the server
   * requests one and accepts the key's signature scheme. */
  if (ctx->cert_chain_count)
    fio_tls13_client_set_cert_chain(
        &conn->state.client,
        ctx->cert_chain,
        ctx->cert_chain_count,
        FIO___TLS13_CRED_KEY(ctx),
        (fio_tls13_signature_algo_e)ctx->signature_algo);
}

/** Called when a new IO is first attached to a valid protocol. */
FIO_SFUNC void fio___tls13_start(fio_io_s *io) {
  fio___tls13_context_s *ctx = (fio___tls13_context_s *)fio_io_tls(io);
  if (!ctx) {
    FIO_LOG_ERROR("TLS 1.3: Context missing for connection!");
    fio_io_close_now(io); /* fail closed, same as the OpenSSL backend */
    return;
  }

  /* Allocate connection state */
  fio___tls13_connection_s *conn = fio___tls13_connection_new(ctx);
  if (!conn) {
    /* never let transport callbacks or cleanup treat the parent context as a
     * connection; release the IO's reference instead. */
    FIO_LOG_ERROR("TLS 1.3: failed to allocate connection state");
    fio_io_tls_set(io, NULL);
    fio___tls13_context_release(ctx);
    fio_io_close_now(io);
    return;
  }

  /* Store the non-owning IO backpointer and connection state. `conn` holds
   * its own context reference, so release the IO's (never the last one). */
  conn->io = io;
  fio_io_tls_set(io, (void *)conn);
  fio___tls13_context_release(ctx);

  FIO_LOG_DDEBUG2("(%d) TLS 1.3: allocated new connection for %p (%s)",
                  (int)fio_thread_getpid(),
                  (void *)io,
                  conn->is_client ? "client" : "server");

  if (conn->is_client) {
    fio___tls13_client_setup(conn);

    /* Generate ClientHello */
    int ch_len = fio_tls13_client_start(&conn->state.client,
                                        conn->out_buf,
                                        conn->out_buf_cap);
    if (ch_len < 0) {
      FIO_LOG_ERROR("TLS 1.3: failed to generate ClientHello");
      fio_io_close_now(io);
      return;
    }
    conn->out_buf_len = (size_t)ch_len;
    conn->out_buf_sent = 0;
  } else {
    fio___tls13_server_setup(conn);
  }
}

/* *****************************************************************************
TLS 1.3 IO Functions - Read
***************************************************************************** */

/** Decrypts one complete TLS record into the supplied destination. */
FIO_SFUNC int fio___tls13_decrypt_record(fio___tls13_connection_s *conn,
                                         uint8_t *dest,
                                         size_t dest_capacity,
                                         const uint8_t *record,
                                         size_t record_len) {
  int result;
  if (conn->is_client) {
    result = fio_tls13_client_decrypt(&conn->state.client,
                                      dest,
                                      dest_capacity,
                                      record,
                                      record_len);
    if (conn->state.client.peer_closed)
      conn->peer_closed = 1;
  } else {
    result = fio_tls13_server_decrypt(&conn->state.server,
                                      dest,
                                      dest_capacity,
                                      record,
                                      record_len);
    if (conn->state.server.peer_closed)
      conn->peer_closed = 1;
  }
  return result;
}

/** Reads post-handshake TLS records through scratch storage.
 *
 * The scratch capacity is bounded by the larger of the caller's remaining
 * buffer and available application-buffer space. Since every TLS record adds
 * wire overhead, all plaintext from the batch fits its selected destination. */
FIO_SFUNC ssize_t fio___tls13_read_connected(fio_socket_i fd,
                                             void *buf,
                                             size_t len,
                                             fio___tls13_connection_s *conn) {
  uint8_t *scratch = fio___tls13_buffer_alloc(1);
  uint8_t *user_buf = (uint8_t *)buf;
  size_t user_written = 0;

  if (conn->peer_closed)
    return 0;
  /* Complete a retained ciphertext fragment before starting a new batch. */
  if (conn->recv_buf_len) {
    if (conn->recv_buf_pos) {
      size_t remaining = conn->recv_buf_len - conn->recv_buf_pos;
      FIO_MEMMOVE(fio___tls13_recv_buf(conn),
                  fio___tls13_recv_buf(conn) + conn->recv_buf_pos,
                  remaining);
      conn->recv_buf_len = remaining;
      conn->recv_buf_pos = 0;
    }
    while (conn->recv_buf_len < FIO_TLS13_RECORD_HEADER_LEN ||
           conn->recv_buf_len <
               FIO_TLS13_RECORD_HEADER_LEN +
                   (((size_t)fio___tls13_recv_buf(conn)[3] << 8) |
                    fio___tls13_recv_buf(conn)[4])) {
      size_t needed;
      if (conn->recv_buf_len < FIO_TLS13_RECORD_HEADER_LEN) {
        needed = FIO_TLS13_RECORD_HEADER_LEN - conn->recv_buf_len;
      } else {
        size_t ciphertext_len = ((size_t)fio___tls13_recv_buf(conn)[3] << 8) |
                                fio___tls13_recv_buf(conn)[4];
        if (ciphertext_len > FIO_TLS13_MAX_CIPHERTEXT_LEN) {
          errno = ECONNRESET;
          return -1;
        }
        needed =
            FIO_TLS13_RECORD_HEADER_LEN + ciphertext_len - conn->recv_buf_len;
      }
      ssize_t read_len = fio_sock_read(fd, (char *)scratch, needed);
      if (read_len <= 0)
        return read_len;
      FIO_MEMCPY(fio___tls13_recv_buf(conn) + conn->recv_buf_len,
                 scratch,
                 (size_t)read_len);
      conn->recv_buf_len += (size_t)read_len;
    }

    size_t record_len = FIO_TLS13_RECORD_HEADER_LEN +
                        (((size_t)fio___tls13_recv_buf(conn)[3] << 8) |
                         fio___tls13_recv_buf(conn)[4]);
    uint8_t *target =
        len >= FIO___TLS13_APP_BUF_CAP ? user_buf : fio___tls13_app_buf(conn);
    size_t target_capacity =
        len >= FIO___TLS13_APP_BUF_CAP ? len : FIO___TLS13_APP_BUF_CAP;
    int decrypted = fio___tls13_decrypt_record(conn,
                                               target,
                                               target_capacity,
                                               fio___tls13_recv_buf(conn),
                                               record_len);
    if (decrypted < 0) {
      errno = ECONNRESET;
      return -1;
    }
    conn->recv_buf_len = 0;
    conn->recv_buf_pos = 0;
    if (conn->peer_closed)
      return 0;
    /* Records without application data (empty records, post-handshake
     * messages) aren't EOF: continue with a fresh batch. */
    if (target == user_buf) {
      if (decrypted)
        return decrypted;
    } else
      conn->app_buf_len = (size_t)decrypted;
  }

  if (conn->app_buf_len > conn->app_buf_pos) {
    size_t available = conn->app_buf_len - conn->app_buf_pos;
    size_t to_copy = len < available ? len : available;
    FIO_MEMCPY(user_buf,
               fio___tls13_app_buf(conn) + conn->app_buf_pos,
               to_copy);
    conn->app_buf_pos += to_copy;
    if (conn->app_buf_pos == conn->app_buf_len) {
      conn->app_buf_len = 0;
      conn->app_buf_pos = 0;
    } else {
      fio_io_on_data_schedule(conn->io);
    }
    return (ssize_t)to_copy;
  }

  size_t app_space = FIO___TLS13_APP_BUF_CAP - conn->app_buf_len;
  size_t read_capacity = len > app_space ? len : app_space;
  if (read_capacity > FIO___TLS13_READ_SCRATCH_CAP)
    read_capacity = FIO___TLS13_READ_SCRATCH_CAP;
  if (!read_capacity)
    return 0;

  ssize_t read_len = fio_sock_read(fd, (char *)scratch, read_capacity);
  if (read_len <= 0)
    return read_len;

  uint8_t *target = len >= app_space ? user_buf : fio___tls13_app_buf(conn);
  size_t target_capacity = len >= app_space ? len : app_space;
  size_t offset = 0;
  size_t produced = 0;
  while ((size_t)read_len - offset >= FIO_TLS13_RECORD_HEADER_LEN) {
    size_t ciphertext_len =
        ((size_t)scratch[offset + 3] << 8) | scratch[offset + 4];
    if (ciphertext_len > FIO_TLS13_MAX_CIPHERTEXT_LEN) {
      errno = ECONNRESET;
      return -1;
    }
    size_t record_len = FIO_TLS13_RECORD_HEADER_LEN + ciphertext_len;
    if ((size_t)read_len - offset < record_len)
      break;
    int decrypted = fio___tls13_decrypt_record(conn,
                                               target + produced,
                                               target_capacity - produced,
                                               scratch + offset,
                                               record_len);
    if (decrypted < 0) {
      if (produced) {
        /* Deliver preceding plaintext now; defer the failing record. */
        read_len = (ssize_t)(offset + record_len);
        fio_io_on_data_schedule(conn->io);
        break;
      }
      errno = ECONNRESET;
      return -1;
    }
    offset += record_len;
    if (conn->peer_closed) {
      /* Do not decrypt ciphertext after a peer close_notify. */
      read_len = (ssize_t)offset;
      break;
    }
    produced += (size_t)decrypted;
  }

  if (offset < (size_t)read_len) {
    /* At most one (complete failing or incomplete) record remains. */
    size_t partial_len = (size_t)read_len - offset;
    FIO_ASSERT_DEBUG(partial_len <= FIO___TLS13_RECV_BUF_CAP,
                     "TLS 1.3: retained record overflows recv_buf");
    FIO_MEMCPY(fio___tls13_recv_buf(conn), scratch + offset, partial_len);
    conn->recv_buf_len = partial_len;
    conn->recv_buf_pos = 0;
  }

  if (target == user_buf)
    user_written = produced;
  else
    conn->app_buf_len = produced;
  if (!produced && conn->peer_closed)
    return 0;

  if (user_written)
    return (ssize_t)user_written;
  if (conn->app_buf_len) {
    size_t to_copy = len < conn->app_buf_len ? len : conn->app_buf_len;
    FIO_MEMCPY(user_buf, fio___tls13_app_buf(conn), to_copy);
    conn->app_buf_pos = to_copy;
    if (conn->app_buf_pos == conn->app_buf_len) {
      conn->app_buf_len = 0;
      conn->app_buf_pos = 0;
    } else {
      fio_io_on_data_schedule(conn->io);
    }
    return (ssize_t)to_copy;
  }
  errno = EWOULDBLOCK;
  return -1;
}

/** Calls the `on_selected` ALPN callback for a negotiated protocol (if any). */
FIO_SFUNC void fio___tls13_alpn_dispatch(fio___tls13_connection_s *conn,
                                         const char *name) {
  if (!name || !name[0] || !conn->ctx->alpn_protocols_len)
    return;
  fio_io_tls_alpn_select(conn->ctx->tls, name, FIO_STRLEN(name), conn->io);
}

/** Called to perform a non-blocking `read`, same as the system call. */
FIO_SFUNC ssize_t fio___tls13_read(fio_socket_i fd,
                                   void *buf,
                                   size_t len,
                                   void *tls_ctx) {
  fio___tls13_connection_s *conn = (fio___tls13_connection_s *)tls_ctx;
  if (!conn || !buf) {
    errno = ENOTCONN;
    return -1;
  }

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
    } else {
      fio_io_on_data_schedule(conn->io);
    }
    return (ssize_t)to_copy;
  }

  if (conn->handshake_complete)
    return fio___tls13_read_connected(fd, buf, len, conn);

  /* Read only through the current record boundary during the handshake.
   * Never consume application records behind the final handshake record:
   * they remain in the socket and need no reactor scheduling to surface. */
  size_t recv_data_len = conn->recv_buf_len - conn->recv_buf_pos;
  if (conn->recv_buf_pos) {
    if (recv_data_len)
      FIO_MEMMOVE(fio___tls13_recv_buf(conn),
                  fio___tls13_recv_buf(conn) + conn->recv_buf_pos,
                  recv_data_len);
    conn->recv_buf_len = recv_data_len;
    conn->recv_buf_pos = 0;
  }
  size_t record_size = FIO_TLS13_RECORD_HEADER_LEN;
  if (recv_data_len >= FIO_TLS13_RECORD_HEADER_LEN) {
    size_t ciphertext_len = ((size_t)fio___tls13_recv_buf(conn)[3] << 8) |
                            fio___tls13_recv_buf(conn)[4];
    if (ciphertext_len > FIO_TLS13_MAX_CIPHERTEXT_LEN) {
      errno = ECONNRESET;
      return -1;
    }
    record_size += ciphertext_len;
  }
  size_t recv_space = record_size - recv_data_len;
  if (!recv_space) { /* A full record already buffered. */
    errno = ECONNRESET;
    return -1;
  }

  /* Read ciphertext through scratch, retaining an incomplete record. */
  uint8_t *read_buf = fio___tls13_buffer_alloc(1);
  errno = 0;
  ssize_t raw_read = fio_sock_read(fd, (char *)read_buf, recv_space);
  if (raw_read <= 0) {
    if (raw_read == 0)
      return 0; /* EOF */
    if (errno == EWOULDBLOCK || errno == EAGAIN)
      return -1;
    return raw_read;
  }
  FIO_MEMCPY(fio___tls13_recv_buf(conn) + conn->recv_buf_len,
             read_buf,
             (size_t)raw_read);
  conn->recv_buf_len += (size_t)raw_read;
  recv_data_len = conn->recv_buf_len - conn->recv_buf_pos;

  /* Process received data */
  uint8_t *recv_ptr = fio___tls13_recv_buf(conn) + conn->recv_buf_pos;
  while (recv_data_len >= FIO_TLS13_RECORD_HEADER_LEN) {
    /* Check if we have a complete record */
    uint16_t record_len = ((uint16_t)recv_ptr[3] << 8) | recv_ptr[4];
    size_t total_record_len = FIO_TLS13_RECORD_HEADER_LEN + record_len;

    if (record_len > FIO_TLS13_MAX_CIPHERTEXT_LEN) {
      errno = ECONNRESET; /* record_overflow, can never fit recv_buf */
      return -1;
    }
    if (recv_data_len < total_record_len)
      break; /* Need more data */

    if (!conn->handshake_complete) {
      /* Process handshake, writing any response straight into the
       * wire-output queue (after already serialized data), with room for
       * the largest flight of the context's certificate chains. */
      size_t out_len = 0;
      int consumed;
      size_t out_need = FIO___TLS13_HS_OUT_NEED(conn->ctx);
      if (fio___tls13_out_reserve(conn, out_need)) {
        FIO_LOG_ERROR("TLS 1.3: handshake output allocation failed");
        errno = ENOBUFS;
        return -1;
      }
      uint8_t *out = conn->out_buf + conn->out_buf_len;
      size_t out_cap = conn->out_buf_cap - conn->out_buf_len;

      if (conn->is_client) {
        consumed = fio_tls13_client_process(&conn->state.client,
                                            recv_ptr,
                                            recv_data_len,
                                            out,
                                            out_cap,
                                            &out_len);
      } else {
        consumed = fio_tls13_server_process(&conn->state.server,
                                            recv_ptr,
                                            recv_data_len,
                                            out,
                                            out_cap,
                                            &out_len);
      }

      /* A complete record is present: consuming nothing means it is
       * malformed (e.g., invalid content type) and retrying would spin. */
      if (consumed <= 0 || (size_t)consumed > recv_data_len) {
        FIO_LOG_DEBUG2("TLS 1.3: handshake processing error");
        errno = ECONNRESET;
        return -1;
      }

      /* Queue the handshake response (already in place) and send it. */
      if (out_len > 0) {
        if (out_len > out_cap) { /* (a broken output length contract) */
          FIO_LOG_ERROR("TLS 1.3: handshake output queue overflow");
          errno = ENOBUFS;
          return -1;
        }
        conn->out_buf_len += out_len;
        (void)fio___tls13_out_flush(fd, conn);
      }

      /* Advance read position (lazy compaction) */
      conn->recv_buf_pos += (size_t)consumed;
      recv_ptr += consumed;
      recv_data_len -= (size_t)consumed;

      /* Check if handshake is complete */
      if (conn->is_client) {
        if (fio_tls13_client_is_connected(&conn->state.client)) {
          conn->handshake_complete = 1;
          FIO_LOG_DEBUG2("TLS 1.3: client handshake complete");
          fio___tls13_alpn_dispatch(
              conn,
              fio_tls13_client_alpn_get(&conn->state.client));
        } else if (fio_tls13_client_is_error(&conn->state.client)) {
          FIO_LOG_DEBUG2("TLS 1.3: client handshake error");
          errno = ECONNRESET;
          return -1;
        }
      } else {
        if (fio_tls13_server_is_connected(&conn->state.server)) {
          conn->handshake_complete = 1;
          FIO_LOG_DEBUG2("TLS 1.3: server handshake complete");
          fio___tls13_alpn_dispatch(
              conn,
              fio_tls13_server_alpn_get(&conn->state.server));
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
        decrypt_capacity = FIO___TLS13_APP_BUF_CAP - conn->app_buf_len;
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
    } else {
      fio_io_on_data_schedule(conn->io);
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
 * the shared wire-output queue and then attempts a single socket write.
 * Handshake data and KeyUpdate responses use the same ordered queue.
 *
 * IMPORTANT: Once data is encrypted, the TLS sequence number is incremented.
 * The function MUST return success (N > 0) after encryption, even if the
 * socket write fails. The encrypted data is buffered and flush() sends later.
 */
FIO_SFUNC ssize_t fio___tls13_write(fio_socket_i fd,
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

  /* Serialized output must drain before new plaintext is accepted: TLS record
   * sequence numbers have already advanced for every queued record. */
  if (fio___tls13_out_flush(fd, conn) < 0 || conn->out_buf_len) {
    errno = EWOULDBLOCK;
    return -1;
  }

  size_t out_pos = 0;

  /* RFC 8446 Section 4.6.3: a requested KeyUpdate response must precede the
   * next application data record. The client/server encrypt functions emit
   * it (under the old keys) together with the record, so the first record
   * reserves room for it. If the response can't be produced, encryption
   * fails and the connection is closed: data is never sent under stale keys
   * while a response is pending. */
  size_t ku_reserve = (conn->is_client ? conn->state.client.key_update_pending
                                       : conn->state.server.key_update_pending)
                          ? FIO___TLS13_KEY_UPDATE_RECORD_LEN
                          : 0;

  /* Encrypt up to four TLS records into the shared wire-output queue. */
  size_t total_plaintext = 0;
  const uint8_t *src = (const uint8_t *)buf;
  size_t remaining_plaintext = len;

  while (remaining_plaintext > 0) {
    size_t out_space = conn->out_buf_cap - out_pos;
    if (out_space < FIO_TLS13_RECORD_HEADER_LEN + 1 + FIO_TLS13_TAG_LEN)
      break;

    size_t chunk = remaining_plaintext;
    if (chunk > FIO_TLS13_MAX_PLAINTEXT_LEN)
      chunk = FIO_TLS13_MAX_PLAINTEXT_LEN;
    size_t max_pt_for_space =
        out_space - FIO_TLS13_RECORD_HEADER_LEN - 1 - FIO_TLS13_TAG_LEN;
    if (max_pt_for_space <= ku_reserve)
      break;
    max_pt_for_space -= ku_reserve;
    if (chunk > max_pt_for_space)
      chunk = max_pt_for_space;

    int out_len;
    if (conn->is_client) {
      out_len = fio_tls13_client_encrypt(&conn->state.client,
                                         conn->out_buf + out_pos,
                                         out_space,
                                         src,
                                         chunk);
    } else {
      out_len = fio_tls13_server_encrypt(&conn->state.server,
                                         conn->out_buf + out_pos,
                                         out_space,
                                         src,
                                         chunk);
    }

    if (out_len < 0) {
      if (total_plaintext > 0)
        break; /* earlier records are valid; the next write reports it */
      FIO_LOG_DEBUG2("TLS 1.3: encryption (or KeyUpdate response) error");
      errno = ECONNRESET;
      return -1;
    }

    ku_reserve = 0; /* a pending KeyUpdate response went out with record 1 */
    out_pos += (size_t)out_len;
    src += chunk;
    remaining_plaintext -= chunk;
    total_plaintext += chunk;
  }

  if (out_pos > 0) {
    conn->out_buf_len = out_pos;
    conn->out_buf_sent = 0;
    if (fio___tls13_out_flush(fd, conn) < 0 && total_plaintext == 0)
      return -1;
  }

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
 * Handshake, KeyUpdate, and encrypted application records share one wire-output
 * queue, so flush only needs to drain that queue. */
FIO_SFUNC int fio___tls13_flush(fio_socket_i fd, void *tls_ctx) {
  fio___tls13_connection_s *conn = (fio___tls13_connection_s *)tls_ctx;
  if (!conn)
    return 0;

  ssize_t flushed = fio___tls13_out_flush(fd, conn);
  if (conn->app_buf_len > conn->app_buf_pos)
    fio_io_on_data_schedule(conn->io);
  if (flushed > 0)
    return (int)flushed;
  if (flushed < 0 || conn->out_buf_len) {
    errno = EWOULDBLOCK;
    return -1;
  }
  return 0;
}

/* *****************************************************************************
TLS 1.3 IO Functions - Finish
***************************************************************************** */

/* close_notify record: header + alert (2) + content type + tag */
#define FIO___TLS13_CLOSE_NOTIFY_RECORD_LEN                                    \
  (FIO_TLS13_RECORD_HEADER_LEN + 2 + 1 + FIO_TLS13_TAG_LEN)

/** Appends an encrypted close_notify alert to the wire-output queue, after any
 * pending records (same keys and sequence order). Compacts the queue when
 * needed. Returns 0 if queued, -1 if skipped (no handshake, no room or
 * encryption failed); pending bytes are always preserved. */
FIO_SFUNC int fio___tls13_queue_close_notify(fio___tls13_connection_s *conn) {
  if (!conn->handshake_complete)
    return -1;
  if (conn->out_buf_cap - conn->out_buf_len <
          FIO___TLS13_CLOSE_NOTIFY_RECORD_LEN &&
      conn->out_buf_sent) { /* drop the sent prefix to make room */
    conn->out_buf_len -= conn->out_buf_sent;
    FIO_MEMMOVE(conn->out_buf,
                conn->out_buf + conn->out_buf_sent,
                conn->out_buf_len);
    conn->out_buf_sent = 0;
  }
  size_t space = conn->out_buf_cap - conn->out_buf_len;
  if (space < FIO___TLS13_CLOSE_NOTIFY_RECORD_LEN)
    return -1;
  const uint8_t alert[2] = {FIO_TLS13_ALERT_LEVEL_WARNING,
                            FIO_TLS13_ALERT_CLOSE_NOTIFY};
  fio_tls13_record_keys_s *keys = conn->is_client
                                      ? &conn->state.client.client_app_keys
                                      : &conn->state.server.server_app_keys;
  int enc_len = fio_tls13_record_encrypt(conn->out_buf + conn->out_buf_len,
                                         space,
                                         alert,
                                         sizeof(alert),
                                         FIO_TLS13_CONTENT_ALERT,
                                         keys);
  if (enc_len <= 0)
    return -1;
  conn->out_buf_len += (size_t)enc_len;
  return 0;
}

/** Called when the IO object finished sending all data before closure.
 *
 * Queues close_notify behind any pending wire output and makes a single
 * non-blocking flush attempt. If close_notify can't be queued (no room or
 * encryption failure), the alert is skipped and the connection just closes. */
FIO_SFUNC void fio___tls13_finish(fio_socket_i fd, void *tls_ctx) {
  fio___tls13_connection_s *conn = (fio___tls13_connection_s *)tls_ctx;
  if (!conn)
    return;
  if (fio___tls13_queue_close_notify(conn) && conn->handshake_complete)
    FIO_LOG_DEBUG2("TLS 1.3: close_notify skipped (no room or encrypt error)");
  if (conn->out_buf_len)
    fio___tls13_out_flush(fd, conn); /* best effort, never blocks */
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
Peer Information Iterator
***************************************************************************** */

/** Returns the next peer certificate for the connection (zero-copy).
 *
 * The iterator is stateless — the position is identified from `dest` alone:
 * a zeroed `dest` (`der.buf == NULL`) starts a new loop at the leaf
 * certificate; otherwise iteration continues at `dest->chain_index + 1`.
 * Returns 0 while data is available, -1 when done / on error. */
FIO_SFUNC int fio___tls13_peer_info_next(fio_socket_i fd,
                                         fio_x509_cert_s *dest,
                                         void *tls_ctx) {
  fio___tls13_connection_s *conn = (fio___tls13_connection_s *)tls_ctx;
  if (!conn || !dest)
    return -1;
  if (!conn->handshake_complete)
    return -1;
  (void)fd;
#if defined(H___FIO_X509___H) && defined(H___FIO_SHA2___H)
  const fio_ubuf_info_s *chain;
  size_t chain_count;
  uint8_t verified;
  if (conn->is_client) {
    chain = conn->state.client.peer_chain.certs;
    chain_count = conn->state.client.peer_chain.count;
    /* skipped verification must not be reported as verified */
    verified = !conn->state.client.skip_cert_verify &&
               conn->state.client.cert_verified &&
               conn->state.client.chain_verified;
  } else {
    chain = conn->state.server.peer_auth.chain.certs;
    chain_count = conn->state.server.peer_auth.chain.count;
    verified = conn->state.server.peer_auth.verified;
  }
  /* Identify the position from `dest` — read `chain_index` BEFORE
   * fio_x509_parse, which zeroes the whole struct.
   * Iteration is capped at 128 certificates (deep-nesting / DoS guard). */
  if ((uint8_t)dest->chain_index & (uint8_t)128U)
    return -1;
  const size_t pos =
      dest->der.buf ? ((size_t)dest->chain_index + 1) : (size_t)0;
  if ((uint8_t)pos & (uint8_t)128U)
    return -1;
  if (pos >= chain_count)
    return -1;
  /* Parse in place: all fio_x509_cert_s views reference the certificate DER
   * stored in the TLS state (peer_chain / peer_auth chain storage), valid
   * for the connection's lifetime. No allocation or copying takes place. */
  if (fio_x509_parse(dest, chain[pos].buf, chain[pos].len) != 0)
    return -1;
  fio_x509_fingerprint(dest);
  dest->verified = verified;
  dest->chain_index = (uint8_t)pos;
  return 0;
#else /* !H___FIO_X509___H || !H___FIO_SHA2___H */
  return -1; /* peer certificate inspection requires the X509 module */
#endif
}

/* *****************************************************************************
TLS 1.3 IO Functions Structure
***************************************************************************** */

/** Returns the TLS 1.3 IO functions (implementation) */
SFUNC fio_io_functions_s fio_tls13_io_functions(void) {
  return (fio_io_functions_s){
      .build_context = fio___tls13_build_context,
      .free_context = fio___tls13_free_context,
      .dup_context = fio___tls13_dup_context,
      .start = fio___tls13_start,
      .read = fio___tls13_read,
      .write = fio___tls13_write,
      .flush = fio___tls13_flush,
      .cleanup = fio___tls13_cleanup,
      .finish = fio___tls13_finish,
      .peer_info_next = fio___tls13_peer_info_next,
  };
}

/* *****************************************************************************
TLS 1.3 Default Setup (when OpenSSL is not available)
***************************************************************************** */

#if (!defined(HAVE_OPENSSL) || defined(FIO_NO_TLS)) &&                         \
    !defined(H___FIO_OPENSSL___H)
/** Setup TLS 1.3 as TLS IO default when OpenSSL is not available (or was
 * disabled with FIO_NO_TLS, which also skips the OpenSSL module). */
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
