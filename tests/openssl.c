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

/* *****************************************************************************
Helpers
***************************************************************************** */

FIO_SFUNC void fio___openssl_test_alpn_cb(fio_io_s *io) { (void)io; }

/* *****************************************************************************
Test: fio_openssl_io_functions returns a populated structure
***************************************************************************** */
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
Main

NOTE: The leak counter warning for fio___openssl_context_s is expected.
The OpenSSL module's free_context uses fio_io_defer() to defer cleanup,
which requires the IO reactor to be running. These unit tests do not run the
reactor, so deferred cleanup tasks may not execute before exit.
***************************************************************************** */
int main(void) {
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
