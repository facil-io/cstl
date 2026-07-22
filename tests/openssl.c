/* *****************************************************************************
Test for 405 openssl.h

Coverage: OpenSSL integration helpers for the fio IO reactor. Tests the
public IO function table, default TLS registration, TLS context lifecycle,
and context building for server/client configurations (self-signed certs,
ALPN, trust stores, URL-based creation). Does NOT cover RSA sign/verify,
X.509 parsing, or PEM parsing — those live in rsa.c, x509.c, and pem.c.
***************************************************************************** */
#define FIO_OPENSSL
#include "test-helpers.h"
#define FIO_IO
#define FIO_URL
#include FIO_INCLUDE_FILE

#if HAVE_OPENSSL

#include <openssl/obj_mac.h> /* NID_X9_62_prime256v1 */

/* *****************************************************************************
Helpers
***************************************************************************** */

FIO_SFUNC void fio___openssl_test_alpn_cb(fio_io_s *io) { (void)io; }

FIO_SFUNC int fio___openssl_test_verify_accept(int preverify_ok,
                                                X509_STORE_CTX *store) {
  (void)preverify_ok;
  (void)store;
  return 1;
}

/** Generates a self-signed ECDSA P-256 certificate with the given CN. */
FIO_SFUNC X509 *fio___openssl_test_make_cert(EVP_PKEY **out_key,
                                             const char *cn) {
  EVP_PKEY *key = NULL;
  EVP_PKEY_CTX *kctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, NULL);
  FIO_ASSERT(kctx != NULL, "EVP_PKEY_CTX_new_id failed");
  FIO_ASSERT(EVP_PKEY_keygen_init(kctx) > 0, "keygen_init failed");
  FIO_ASSERT(
      EVP_PKEY_CTX_set_ec_paramgen_curve_nid(kctx, NID_X9_62_prime256v1) > 0,
      "curve selection failed");
  FIO_ASSERT(EVP_PKEY_keygen(kctx, &key) > 0, "keygen failed");
  EVP_PKEY_CTX_free(kctx);

  X509 *x = X509_new();
  FIO_ASSERT(x != NULL, "X509_new failed");
  X509_set_version(x, 2);
  ASN1_INTEGER_set(X509_get_serialNumber(x), 1);
  X509_gmtime_adj(X509_getm_notBefore(x), -3600);
  X509_gmtime_adj(X509_getm_notAfter(x), 3600);
  X509_NAME *name = X509_get_subject_name(x);
  X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC,
                             (const unsigned char *)cn, -1, -1, 0);
  X509_set_issuer_name(x, name);
  X509_set_pubkey(x, key);
  FIO_ASSERT(X509_sign(x, key, EVP_sha256()) > 0, "X509_sign failed");
  *out_key = key;
  return x;
}

/* *****************************************************************************
Test: TLS context overhead and fio_openssl_io_functions
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, openssl_context_overhead)(void) {
  FIO_LOG_INFO("OpenSSL global TLS server context overhead: %zu Bytes",
               sizeof(fio___openssl_context_s));
  FIO_LOG_INFO("OpenSSL per-connection TLS context overhead: %zu Bytes",
               sizeof(fio___openssl_connection_s));
}

FIO_SFUNC void FIO_NAME_TEST(stl, openssl_io_functions)(void) {
  fio_io_functions_s funcs = fio_openssl_io_functions();

  FIO_ASSERT(funcs.build_context != NULL,
             "fio_openssl_io_functions: build_context should not be NULL");
  FIO_ASSERT(funcs.free_context != NULL,
             "fio_openssl_io_functions: free_context should not be NULL");
  FIO_ASSERT(funcs.start != NULL,
             "fio_openssl_io_functions: start should not be NULL");
  FIO_ASSERT(funcs.read != NULL,
             "fio_openssl_io_functions: read should not be NULL");
  FIO_ASSERT(funcs.write != NULL,
             "fio_openssl_io_functions: write should not be NULL");
  FIO_ASSERT(funcs.flush != NULL,
             "fio_openssl_io_functions: flush should not be NULL");
  FIO_ASSERT(funcs.finish != NULL,
             "fio_openssl_io_functions: finish should not be NULL");
  FIO_ASSERT(funcs.cleanup != NULL,
             "fio_openssl_io_functions: cleanup should not be NULL");
  FIO_ASSERT(funcs.peer_info_next != NULL,
             "fio_openssl_io_functions: peer_info_next should not be NULL");
}

/* *****************************************************************************
Test: TLS context reference counting
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, openssl_tls_context)(void) {
  fio_io_tls_s *tls = fio_io_tls_new();
  FIO_ASSERT(tls != NULL, "fio_io_tls_new() should return non-NULL");

  fio_io_tls_s *tls_dup = fio_io_tls_dup(tls);
  FIO_ASSERT(tls_dup == tls, "fio_io_tls_dup() should return the same pointer");

  fio_io_tls_free(tls_dup);
  fio_io_tls_free(tls);
}

/* *****************************************************************************
Test: OpenSSL registered itself as the default TLS implementation
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, openssl_default_registration)(void) {
  fio_io_functions_s defaults = fio_io_tls_default_functions(NULL);

  FIO_ASSERT(defaults.build_context != NULL,
             "Default TLS functions should have build_context set");
  FIO_ASSERT(defaults.read != NULL,
             "Default TLS functions should have read set");
  FIO_ASSERT(defaults.write != NULL,
             "Default TLS functions should have write set");
}

/* *****************************************************************************
Test: Server context with auto-generated self-signed certificate
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, openssl_self_signed)(void) {
  fio_io_functions_s funcs = fio_openssl_io_functions();

  fio_io_tls_s *tls = fio_io_tls_new();
  FIO_ASSERT(tls != NULL, "fio_io_tls_new() failed");
  FIO_ASSERT(fio_io_tls_cert_count(tls) == 0,
             "New TLS context should have 0 certificates");

  void *ctx = funcs.build_context(tls, 0); /* server */
  FIO_ASSERT(ctx != NULL,
             "build_context() should succeed with self-signed cert");

  funcs.free_context(ctx);
  fio_io_tls_free(tls);
}

/* *****************************************************************************
Test: Server context with named self-signed certificate
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, openssl_self_signed_named)(void) {
  fio_io_functions_s funcs = fio_openssl_io_functions();

  fio_io_tls_s *tls = fio_io_tls_new();
  FIO_ASSERT(tls != NULL, "fio_io_tls_new() failed");

  fio_io_tls_cert_add(tls, "test.example.com", NULL, NULL, NULL);
  FIO_ASSERT(fio_io_tls_cert_count(tls) == 1,
             "TLS context should have 1 certificate entry");

  void *ctx = funcs.build_context(tls, 0);
  FIO_ASSERT(ctx != NULL,
             "build_context() should succeed with named self-signed cert");

  funcs.free_context(ctx);
  fio_io_tls_free(tls);
}

/* *****************************************************************************
Test: Client context creation
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, openssl_client_context)(void) {
  fio_io_functions_s funcs = fio_openssl_io_functions();

  fio_io_tls_s *tls = fio_io_tls_new();
  FIO_ASSERT(tls != NULL, "fio_io_tls_new() failed");

  void *ctx = funcs.build_context(tls, 1); /* client */
  FIO_ASSERT(ctx != NULL, "build_context() should succeed for client");

  funcs.free_context(ctx);
  fio_io_tls_free(tls);
}

/* *****************************************************************************
Test: ALPN protocol configuration
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, openssl_alpn)(void) {
  fio_io_functions_s funcs = fio_openssl_io_functions();

  fio_io_tls_s *tls = fio_io_tls_new();
  FIO_ASSERT(tls != NULL, "fio_io_tls_new() failed");
  FIO_ASSERT(fio_io_tls_alpn_count(tls) == 0,
             "New TLS context should have 0 ALPN protocols");

  fio_io_tls_alpn_add(tls, "h2", fio___openssl_test_alpn_cb);
  FIO_ASSERT(fio_io_tls_alpn_count(tls) == 1,
             "TLS context should have 1 ALPN protocol after first add");

  fio_io_tls_alpn_add(tls, "http/1.1", fio___openssl_test_alpn_cb);
  FIO_ASSERT(fio_io_tls_alpn_count(tls) == 2,
             "TLS context should have 2 ALPN protocols after second add");

  void *ctx = funcs.build_context(tls, 0);
  FIO_ASSERT(ctx != NULL, "build_context() should succeed with ALPN protocols");

  funcs.free_context(ctx);
  fio_io_tls_free(tls);
}

/* *****************************************************************************
Test: ALPN edge cases
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, openssl_alpn_edge_cases)(void) {
  fio_io_tls_s *tls = fio_io_tls_new();
  FIO_ASSERT(tls != NULL, "fio_io_tls_new() failed");

  fio_io_tls_alpn_add(tls, NULL, fio___openssl_test_alpn_cb);
  FIO_ASSERT(fio_io_tls_alpn_count(tls) == 0,
             "NULL ALPN protocol should be ignored");

  fio_io_tls_alpn_add(tls, "valid-protocol", fio___openssl_test_alpn_cb);
  FIO_ASSERT(fio_io_tls_alpn_count(tls) == 1,
             "Valid ALPN protocol should be added");

  fio_io_tls_alpn_add(tls, "another-protocol", NULL);
  FIO_ASSERT(fio_io_tls_alpn_count(tls) == 2,
             "ALPN with NULL callback should be added");

  fio_io_tls_free(tls);
}

/* *****************************************************************************
Test: Trust store entry counting
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, openssl_trust_explicit)(void) {
  fio_io_tls_s *tls = fio_io_tls_new();
  FIO_ASSERT(tls != NULL, "fio_io_tls_new() failed");
  FIO_ASSERT(fio_io_tls_trust_count(tls) == 0,
             "New TLS context should have 0 trust certificates");

  fio_io_tls_trust_add(tls, "/nonexistent/ca-bundle.pem");
  FIO_ASSERT(fio_io_tls_trust_count(tls) == 1,
             "TLS context should have 1 trust entry after adding explicit path");

  fio_io_tls_trust_add(tls, "/another/ca.pem");
  FIO_ASSERT(fio_io_tls_trust_count(tls) == 2,
             "TLS context should have 2 trust entries");

  fio_io_tls_free(tls);
}

/* *****************************************************************************
Test: Multiple certificate entries
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, openssl_multi_cert)(void) {
  fio_io_tls_s *tls = fio_io_tls_new();
  FIO_ASSERT(tls != NULL, "fio_io_tls_new() failed");

  fio_io_tls_cert_add(tls, "www.example.com", NULL, NULL, NULL);
  fio_io_tls_cert_add(tls, "api.example.com", NULL, NULL, NULL);
  fio_io_tls_cert_add(tls, "admin.example.com", NULL, NULL, NULL);

  FIO_ASSERT(fio_io_tls_cert_count(tls) == 3,
             "TLS context should have 3 certificate entries");

  fio_io_tls_free(tls);
}

/* *****************************************************************************
Test: fio_io_tls_each iteration
***************************************************************************** */
FIO_SFUNC int fio___openssl_test_each_cert_count;
FIO_SFUNC int fio___openssl_test_each_alpn_count;
FIO_SFUNC int fio___openssl_test_each_trust_count;

FIO_SFUNC int fio___openssl_test_each_cert_cb(struct fio_io_tls_each_s *e,
                                              const char *server_name,
                                              const char *public_cert_file,
                                              const char *private_key_file,
                                              const char *pk_password) {
  (void)e;
  (void)server_name;
  (void)public_cert_file;
  (void)private_key_file;
  (void)pk_password;
  ++fio___openssl_test_each_cert_count;
  return 0;
}

FIO_SFUNC int fio___openssl_test_each_alpn_cb(struct fio_io_tls_each_s *e,
                                              const char *protocol_name,
                                              void (*on_selected)(fio_io_s *)) {
  (void)e;
  (void)protocol_name;
  (void)on_selected;
  ++fio___openssl_test_each_alpn_count;
  return 0;
}

FIO_SFUNC int fio___openssl_test_each_trust_cb(struct fio_io_tls_each_s *e,
                                               const char *public_cert_file) {
  (void)e;
  (void)public_cert_file;
  ++fio___openssl_test_each_trust_count;
  return 0;
}

FIO_SFUNC void FIO_NAME_TEST(stl, openssl_tls_each)(void) {
  fio_io_tls_s *tls = fio_io_tls_new();
  FIO_ASSERT(tls != NULL, "fio_io_tls_new() failed");

  fio_io_tls_cert_add(tls, "host1.example.com", NULL, NULL, NULL);
  fio_io_tls_cert_add(tls, "host2.example.com", NULL, NULL, NULL);
  fio_io_tls_alpn_add(tls, "h2", fio___openssl_test_alpn_cb);
  fio_io_tls_alpn_add(tls, "http/1.1", fio___openssl_test_alpn_cb);
  fio_io_tls_alpn_add(tls, "spdy/3.1", fio___openssl_test_alpn_cb);
  fio_io_tls_trust_add(tls, NULL);          /* system trust store */
  fio_io_tls_trust_add(tls, "/some/ca.pem"); /* explicit path */

  fio___openssl_test_each_cert_count = 0;
  fio___openssl_test_each_alpn_count = 0;
  fio___openssl_test_each_trust_count = 0;

  int result = fio_io_tls_each(tls,
                               .udata = NULL,
                               .each_cert = fio___openssl_test_each_cert_cb,
                               .each_alpn = fio___openssl_test_each_alpn_cb,
                               .each_trust = fio___openssl_test_each_trust_cb);

  FIO_ASSERT(result == 0, "fio_io_tls_each() should return 0 on success");
  FIO_ASSERT(fio___openssl_test_each_cert_count == 2,
             "fio_io_tls_each() should iterate 2 certificates, got %d",
             fio___openssl_test_each_cert_count);
  FIO_ASSERT(fio___openssl_test_each_alpn_count == 3,
             "fio_io_tls_each() should iterate 3 ALPN protocols, got %d",
             fio___openssl_test_each_alpn_count);
  FIO_ASSERT(fio___openssl_test_each_trust_count == 2,
             "fio_io_tls_each() should iterate 2 trust entries, got %d",
             fio___openssl_test_each_trust_count);

  fio_io_tls_free(tls);
}

/* *****************************************************************************
Test: Full server/client configuration round-trip
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, openssl_full_config)(void) {
  fio_io_functions_s funcs = fio_openssl_io_functions();

  /* Server: self-signed cert + ALPN */
  {
    fio_io_tls_s *tls = fio_io_tls_new();
    FIO_ASSERT(tls != NULL, "fio_io_tls_new() failed");

    fio_io_tls_cert_add(tls, "secure.example.com", NULL, NULL, NULL);
    fio_io_tls_alpn_add(tls, "h2", fio___openssl_test_alpn_cb);
    fio_io_tls_alpn_add(tls, "http/1.1", fio___openssl_test_alpn_cb);

    void *server_ctx = funcs.build_context(tls, 0);
    FIO_ASSERT(server_ctx != NULL,
               "build_context() should succeed with server config");
    funcs.free_context(server_ctx);

    fio_io_tls_free(tls);
  }

  /* Client: ALPN only, no trust = no verification */
  {
    fio_io_tls_s *tls = fio_io_tls_new();
    FIO_ASSERT(tls != NULL, "fio_io_tls_new() failed");

    fio_io_tls_alpn_add(tls, "h2", fio___openssl_test_alpn_cb);
    fio_io_tls_alpn_add(tls, "http/1.1", fio___openssl_test_alpn_cb);

    void *client_ctx = funcs.build_context(tls, 1);
    FIO_ASSERT(client_ctx != NULL,
               "build_context() should succeed with client config");
    funcs.free_context(client_ctx);

    fio_io_tls_free(tls);
  }
}

/* *****************************************************************************
Test: TLS context creation from URL schemes
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, openssl_tls_from_url)(void) {
  {
    fio_url_s url = fio_url_parse("https://example.com:443/path", 28);
    fio_io_tls_s *tls = fio_io_tls_from_url(NULL, url);
    FIO_ASSERT(tls != NULL,
               "fio_io_tls_from_url() should create TLS for https URL");
    fio_io_tls_free(tls);
  }

  {
    fio_url_s url = fio_url_parse("http://example.com:80/path", 26);
    fio_io_tls_s *tls = fio_io_tls_from_url(NULL, url);
    FIO_ASSERT(tls == NULL,
               "fio_io_tls_from_url() should return NULL for http URL");
  }

  {
    fio_url_s url = fio_url_parse("wss://example.com/ws", 20);
    fio_io_tls_s *tls = fio_io_tls_from_url(NULL, url);
    FIO_ASSERT(tls != NULL,
               "fio_io_tls_from_url() should create TLS for wss URL");
    fio_io_tls_free(tls);
  }
}

/* *****************************************************************************
Test: stateless peer certificate iteration (fio___openssl_peer_info_next)

Uses a real in-memory TLS handshake (BIO pair) with a 2-certificate chain.
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, openssl_peer_info_next)(void) {
  EVP_PKEY *leaf_key = NULL, *ca_key = NULL;
  X509 *leaf = fio___openssl_test_make_cert(&leaf_key, "leaf.local");
  X509 *ca = fio___openssl_test_make_cert(&ca_key, "ca.local");

  /* Server context: leaf + extra chain cert (2-certificate chain) */
  SSL_CTX *sctx = SSL_CTX_new(TLS_server_method());
  FIO_ASSERT(sctx != NULL, "server SSL_CTX_new failed");
  FIO_ASSERT(SSL_CTX_use_certificate(sctx, leaf) == 1, "use_certificate");
  FIO_ASSERT(SSL_CTX_use_PrivateKey(sctx, leaf_key) == 1, "use_PrivateKey");
  FIO_ASSERT(SSL_CTX_add0_chain_cert(sctx, ca) == 1,
             "add0_chain_cert failed"); /* ctx owns `ca` on success */
  SSL_CTX_set_verify(sctx,
                     SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT,
                     fio___openssl_test_verify_accept);

  /* Client context: no server verification, but send a client certificate. */
  SSL_CTX *cctx = SSL_CTX_new(TLS_client_method());
  FIO_ASSERT(cctx != NULL, "client SSL_CTX_new failed");
  SSL_CTX_set_verify(cctx, SSL_VERIFY_NONE, NULL);
  FIO_ASSERT(SSL_CTX_use_certificate(cctx, leaf) == 1,
             "client use_certificate");
  FIO_ASSERT(SSL_CTX_use_PrivateKey(cctx, leaf_key) == 1,
             "client use_PrivateKey");

  /* In-memory full handshake over a BIO pair */
  BIO *sbio = NULL, *cbio = NULL;
  FIO_ASSERT(BIO_new_bio_pair(&sbio, 0, &cbio, 0) == 1, "BIO pair failed");
  SSL *sssl = SSL_new(sctx);
  SSL *cssl = SSL_new(cctx);
  FIO_ASSERT(sssl && cssl, "SSL_new failed");
  SSL_set_bio(sssl, sbio, sbio);
  SSL_set_bio(cssl, cbio, cbio);
  SSL_set_accept_state(sssl);
  SSL_set_connect_state(cssl);
  for (int i = 0;
       i < 4096 && (!SSL_is_init_finished(cssl) || !SSL_is_init_finished(sssl));
       ++i) {
    SSL_do_handshake(cssl);
    SSL_do_handshake(sssl);
  }
  FIO_ASSERT(SSL_is_init_finished(cssl) && SSL_is_init_finished(sssl),
             "in-memory handshake failed");

  /* OpenSSL documents an asymmetric raw-chain layout: client-side calls
   * include the server leaf, while server-side calls exclude the client leaf. */
  STACK_OF(X509) *client_chain = SSL_get_peer_cert_chain(cssl);
  FIO_ASSERT(client_chain && sk_X509_num(client_chain) == 2,
             "client raw peer chain should contain leaf + extra chain cert");
  FIO_ASSERT(sk_X509_value(client_chain, 0) &&
                 X509_cmp(sk_X509_value(client_chain, 0), leaf) == 0,
             "client raw peer chain should include the leaf at index 0");
  FIO_ASSERT(sk_X509_value(client_chain, 1) &&
                 X509_cmp(sk_X509_value(client_chain, 1), ca) == 0,
             "client raw peer chain should preserve the remaining chain");
  STACK_OF(X509) *server_chain = SSL_get_peer_cert_chain(sssl);
  FIO_ASSERT(!server_chain || sk_X509_num(server_chain) == 0,
             "server raw peer chain should exclude the leaf-only client cert");

  /* Fabricate the per-connection wrapper (client iterates server chain) */
  fio___openssl_connection_s conn;
  FIO_MEMSET(&conn, 0, sizeof(conn));
  conn.ssl = cssl;
  conn.handshake_complete = 1;

  /* Expected leaf DER (canonical re-encoding == received bytes) */
  int leaf_der_len_i = i2d_X509(leaf, NULL);
  FIO_ASSERT(leaf_der_len_i > 0, "i2d_X509 sizing failed");
  size_t leaf_der_len = (size_t)leaf_der_len_i;
  uint8_t *leaf_der = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, leaf_der_len, 0);
  FIO_ASSERT(leaf_der != NULL, "allocation failed");
  uint8_t *p = leaf_der;
  FIO_ASSERT(i2d_X509(leaf, &p) == leaf_der_len_i, "i2d_X509 failed");

  fio_x509_cert_s info;

  /* Handshake incomplete -> -1 */
  conn.handshake_complete = 0;
  FIO_MEMSET(&info, 0, sizeof(info));
  FIO_ASSERT(fio___openssl_peer_info_next(-1, &info, &conn) == -1,
             "handshake incomplete should return -1");
  conn.handshake_complete = 1;

  /* NULL dest is an error — there is no hidden reset channel */
  FIO_ASSERT(fio___openssl_peer_info_next(-1, NULL, &conn) == -1,
             "NULL dest should return -1");

  /* A zeroed dest starts a new loop at the leaf certificate */
  FIO_MEMSET(&info, 0, sizeof(info));
  FIO_ASSERT(fio___openssl_peer_info_next(-1, &info, &conn) == 0,
             "peer_info_next failed for leaf certificate");
  FIO_ASSERT(info.chain_index == 0, "leaf should have chain_index 0");
  FIO_ASSERT(info.der.buf != NULL && info.der.len == leaf_der_len &&
                 !FIO_MEMCMP(info.der.buf, leaf_der, leaf_der_len),
             "leaf DER mismatch (arena staging broken)");
  FIO_ASSERT(info.cn.buf != NULL && info.cn.len == 10 &&
                 !FIO_MEMCMP(info.cn.buf, "leaf.local", 10),
             "leaf subject CN mismatch");
  const uint8_t *leaf_slot = info.der.buf; /* arena slot of the leaf */

  /* The next call steps forward using the data in `dest` alone */
  FIO_ASSERT(fio___openssl_peer_info_next(-1, &info, &conn) == 0,
             "peer_info_next failed for second certificate");
  FIO_ASSERT(info.chain_index == 1, "second cert should have chain_index 1");
  FIO_ASSERT(info.der.buf != leaf_slot,
             "each call must stage into a FRESH arena slot");
  FIO_ASSERT(info.cn.buf != NULL && info.cn.len == 8 &&
                 !FIO_MEMCMP(info.cn.buf, "ca.local", 8),
             "second cert subject CN mismatch");

  /* Arena freshness: the leaf's staged DER survives subsequent calls */
  FIO_ASSERT(!FIO_MEMCMP(leaf_slot, leaf_der, leaf_der_len),
             "leaf arena slot was overwritten by the next call");

  /* Iteration ends with -1 */
  FIO_ASSERT(fio___openssl_peer_info_next(-1, &info, &conn) == -1,
             "iteration should end after the last certificate");

  /* The server-side chain API excludes the client leaf on OpenSSL. */
  fio___openssl_connection_s server_conn;
  FIO_MEMSET(&server_conn, 0, sizeof(server_conn));
  server_conn.ssl = sssl;
  server_conn.handshake_complete = 1;
  FIO_MEMSET(&info, 0, sizeof(info));
  FIO_ASSERT(fio___openssl_peer_info_next(-1, &info, &server_conn) == 0,
             "server peer_info_next failed for client leaf certificate");
  FIO_ASSERT(info.chain_index == 0 && info.der.len == leaf_der_len &&
                 !FIO_MEMCMP(info.der.buf, leaf_der, leaf_der_len),
             "server-side client leaf DER mismatch");
  FIO_ASSERT(fio___openssl_peer_info_next(-1, &info, &server_conn) == -1,
             "server-side client chain should contain only the leaf");

  /* Restarting a loop only requires zeroing the struct */
  FIO_MEMSET(&info, 0, sizeof(info));
  FIO_ASSERT(fio___openssl_peer_info_next(-1, &info, &conn) == 0 &&
                 info.chain_index == 0 && info.der.len == leaf_der_len &&
                 !FIO_MEMCMP(info.der.buf, leaf_der, leaf_der_len),
             "zeroed dest should restart the loop at the leaf");

  /* Interleaved loops on the same connection do not interfere */
  fio_x509_cert_s a, b;
  FIO_MEMSET(&a, 0, sizeof(a));
  FIO_MEMSET(&b, 0, sizeof(b));
  FIO_ASSERT(fio___openssl_peer_info_next(-1, &a, &conn) == 0 &&
                 a.chain_index == 0,
             "loop A failed at leaf");
  FIO_ASSERT(fio___openssl_peer_info_next(-1, &b, &conn) == 0 &&
                 b.chain_index == 0,
             "loop B failed at leaf");
  FIO_ASSERT(fio___openssl_peer_info_next(-1, &a, &conn) == 0 &&
                 a.chain_index == 1,
             "loop A should step to the second certificate");
  FIO_ASSERT(b.chain_index == 0, "loop B state must be untouched by loop A");
  FIO_ASSERT(fio___openssl_peer_info_next(-1, &a, &conn) == -1,
             "loop A should end");
  FIO_ASSERT(fio___openssl_peer_info_next(-1, &b, &conn) == 0 &&
                 b.chain_index == 1,
             "loop B should step to the second certificate");
  FIO_ASSERT(fio___openssl_peer_info_next(-1, &b, &conn) == -1,
             "loop B should end");

  /* Iteration is capped at 128 certificates (deep-nesting / DoS guard) */
  FIO_MEMSET(&info, 0, sizeof(info));
  info.der.buf = (uint8_t *)leaf_slot; /* continue from a forged position */
  info.chain_index = 127;
  FIO_ASSERT(fio___openssl_peer_info_next(-1, &info, &conn) == -1,
             "iteration must stop at the 128-certificate cap");
  info.chain_index = 200;
  FIO_ASSERT(fio___openssl_peer_info_next(-1, &info, &conn) == -1,
             "chain_index >= 128 must return -1");

  /* Cleanup */
  FIO_MEM_FREE(leaf_der, leaf_der_len);
  SSL_free(sssl);
  SSL_free(cssl);
  SSL_CTX_free(sctx); /* frees `ca` (ownership transferred) */
  SSL_CTX_free(cctx);
  X509_free(leaf);
  EVP_PKEY_free(leaf_key);
  EVP_PKEY_free(ca_key);
}

/* *****************************************************************************
Main

NOTE: The leak counter warning for fio___openssl_context_s is expected.
The OpenSSL module's free_context uses fio_io_defer() to defer cleanup,
which requires the IO reactor to be running. These unit tests do not run the
reactor, so deferred cleanup tasks may not execute before exit.
***************************************************************************** */
int main(void) {
  FIO_NAME_TEST(stl, openssl_context_overhead)();
  FIO_NAME_TEST(stl, openssl_io_functions)();
  FIO_NAME_TEST(stl, openssl_tls_context)();
  FIO_NAME_TEST(stl, openssl_default_registration)();
  FIO_NAME_TEST(stl, openssl_self_signed)();
  FIO_NAME_TEST(stl, openssl_self_signed_named)();
  FIO_NAME_TEST(stl, openssl_client_context)();
  FIO_NAME_TEST(stl, openssl_alpn)();
  FIO_NAME_TEST(stl, openssl_alpn_edge_cases)();
  FIO_NAME_TEST(stl, openssl_trust_explicit)();
  FIO_NAME_TEST(stl, openssl_multi_cert)();
  FIO_NAME_TEST(stl, openssl_tls_each)();
  FIO_NAME_TEST(stl, openssl_full_config)();
  FIO_NAME_TEST(stl, openssl_tls_from_url)();
  FIO_NAME_TEST(stl, openssl_peer_info_next)();
  return 0;
}

#else /* !HAVE_OPENSSL */

/* *****************************************************************************
OpenSSL unavailable - skip gracefully
***************************************************************************** */
int main(void) {
  fprintf(stderr, "* OpenSSL not available, skipping openssl integration tests.\n");
  return 0;
}

#endif /* HAVE_OPENSSL */
