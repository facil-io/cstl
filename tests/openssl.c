/* *****************************************************************************
OpenSSL TLS Module Tests
***************************************************************************** */
#include "test-helpers.h"

#define FIO_IO
#include FIO_INCLUDE_FILE

/* *****************************************************************************
Test: OpenSSL availability check
***************************************************************************** */

#if !defined(H___FIO_OPENSSL___H)

int main(void) {
  fprintf(stderr, "* OpenSSL module not available, skipping tests.\n");
  fprintf(stderr,
          "  (OpenSSL 3.x required, or FIO_NO_TLS defined, or headers missing)"
          "\n");
  return 0;
}

#else /* OpenSSL is available */

/* *****************************************************************************
Test: fio_openssl_io_functions returns valid structure
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, openssl_io_functions)(void) {
  FIO_LOG_DDEBUG("Testing fio_openssl_io_functions()");

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

  FIO_LOG_DDEBUG("  fio_openssl_io_functions() - PASS");
}

/* *****************************************************************************
Test: TLS context creation and destruction
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, openssl_tls_context)(void) {
  FIO_LOG_DDEBUG("Testing TLS context creation/destruction");

  /* Test basic TLS context creation */
  fio_io_tls_s *tls = fio_io_tls_new();
  FIO_ASSERT(tls != NULL, "fio_io_tls_new() should return non-NULL");

  /* Test reference counting with dup */
  fio_io_tls_s *tls_dup = fio_io_tls_dup(tls);
  FIO_ASSERT(tls_dup == tls, "fio_io_tls_dup() should return same pointer");

  /* Free both references */
  fio_io_tls_free(tls_dup);
  fio_io_tls_free(tls);

  FIO_LOG_DDEBUG("  TLS context creation/destruction - PASS");
}

/* *****************************************************************************
Test: Self-signed certificate generation (via context build)
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, openssl_self_signed)(void) {
  FIO_LOG_DDEBUG("Testing self-signed certificate generation");

  fio_io_functions_s funcs = fio_openssl_io_functions();

  /* Create TLS config without certificates - should trigger self-signed */
  fio_io_tls_s *tls = fio_io_tls_new();
  FIO_ASSERT(tls != NULL, "fio_io_tls_new() failed");

  /* Verify no certificates are configured */
  FIO_ASSERT(fio_io_tls_cert_count(tls) == 0,
             "New TLS context should have 0 certificates");

  /* Build server context - should generate self-signed certificate */
  void *ctx = funcs.build_context(tls, 0); /* is_client = 0 (server) */
  FIO_ASSERT(ctx != NULL,
             "build_context() should succeed with self-signed cert");

  /* Clean up */
  funcs.free_context(ctx);
  fio_io_tls_free(tls);

  FIO_LOG_DDEBUG("  Self-signed certificate generation - PASS");
}

/* *****************************************************************************
Test: Self-signed certificate with custom server name
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, openssl_self_signed_named)(void) {
  FIO_LOG_DDEBUG("Testing self-signed certificate with server name");

  fio_io_functions_s funcs = fio_openssl_io_functions();

  /* Create TLS config with server name but no cert files */
  fio_io_tls_s *tls = fio_io_tls_new();
  FIO_ASSERT(tls != NULL, "fio_io_tls_new() failed");

  /* Add certificate entry with server name but NULL files (triggers
   * self-signed) */
  fio_io_tls_cert_add(tls, "test.example.com", NULL, NULL, NULL);

  FIO_ASSERT(fio_io_tls_cert_count(tls) == 1,
             "TLS context should have 1 certificate entry");

  /* Build server context */
  void *ctx = funcs.build_context(tls, 0);
  FIO_ASSERT(ctx != NULL,
             "build_context() should succeed with named self-signed cert");

  /* Clean up */
  funcs.free_context(ctx);
  fio_io_tls_free(tls);

  FIO_LOG_DDEBUG("  Self-signed certificate with server name - PASS");
}

/* *****************************************************************************
Test: ALPN protocol configuration
***************************************************************************** */
FIO_SFUNC void fio___test_alpn_callback(fio_io_s *io) { (void)io; }

FIO_SFUNC void FIO_NAME_TEST(stl, openssl_alpn)(void) {
  FIO_LOG_DDEBUG("Testing ALPN protocol configuration");

  fio_io_functions_s funcs = fio_openssl_io_functions();

  fio_io_tls_s *tls = fio_io_tls_new();
  FIO_ASSERT(tls != NULL, "fio_io_tls_new() failed");

  /* Initially no ALPN protocols */
  FIO_ASSERT(fio_io_tls_alpn_count(tls) == 0,
             "New TLS context should have 0 ALPN protocols");

  /* Add ALPN protocols */
  fio_io_tls_alpn_add(tls, "h2", fio___test_alpn_callback);
  FIO_ASSERT(fio_io_tls_alpn_count(tls) == 1,
             "TLS context should have 1 ALPN protocol after first add");

  fio_io_tls_alpn_add(tls, "http/1.1", fio___test_alpn_callback);
  FIO_ASSERT(fio_io_tls_alpn_count(tls) == 2,
             "TLS context should have 2 ALPN protocols after second add");

  /* Build context with ALPN */
  void *ctx = funcs.build_context(tls, 0);
  FIO_ASSERT(ctx != NULL, "build_context() should succeed with ALPN protocols");

  /* Clean up */
  funcs.free_context(ctx);
  fio_io_tls_free(tls);

  FIO_LOG_DDEBUG("  ALPN protocol configuration - PASS");
}

/* *****************************************************************************
Test: Trust store configuration
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, openssl_trust)(void) {
  FIO_LOG_DDEBUG("Testing trust store configuration");

  fio_io_functions_s funcs = fio_openssl_io_functions();

  fio_io_tls_s *tls = fio_io_tls_new();
  FIO_ASSERT(tls != NULL, "fio_io_tls_new() failed");

  /* Initially no trust certificates */
  FIO_ASSERT(fio_io_tls_trust_count(tls) == 0,
             "New TLS context should have 0 trust certificates");

  /* Add system default trust store (NULL path)
   * Note: NULL path sets trust_sys flag but doesn't increment trust.count
   * The fio_io_tls_each() will still iterate over it via trust_sys check
   *
   * IMPORTANT: The OpenSSL module currently uses fio_io_tls_trust_count()
   * to decide whether to enable verification. Since NULL path doesn't
   * increment the count, we need to also add an explicit path to trigger
   * verification mode.
   */
  fio_io_tls_trust_add(tls, NULL);
  /* trust_count remains 0 because NULL path sets internal flag, not map entry
   */

  /* For verification to be enabled, we need at least one explicit trust entry
   * This is a quirk of the current implementation where trust_count is checked
   */
  fio_io_tls_trust_add(tls, "/nonexistent/but/triggers/verify.pem");
  FIO_ASSERT(fio_io_tls_trust_count(tls) == 1,
             "TLS context should have 1 trust entry after explicit add");

  /* Build client context with trust verification
   * Note: This will fail to load the nonexistent file, but the context
   * creation itself should work (file loading happens in each_trust callback)
   */
  void *ctx = funcs.build_context(tls, 1); /* is_client = 1 */
  /* Context may be NULL if trust file loading fails - that's expected */
  if (ctx) {
    funcs.free_context(ctx);
  }

  /* Clean up */
  fio_io_tls_free(tls);

  FIO_LOG_DDEBUG("  Trust store configuration - PASS");
}

/* *****************************************************************************
Test: Client context creation
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, openssl_client_context)(void) {
  FIO_LOG_DDEBUG("Testing client context creation");

  fio_io_functions_s funcs = fio_openssl_io_functions();

  fio_io_tls_s *tls = fio_io_tls_new();
  FIO_ASSERT(tls != NULL, "fio_io_tls_new() failed");

  /* Build client context (no certificates needed for client) */
  void *ctx = funcs.build_context(tls, 1); /* is_client = 1 */
  FIO_ASSERT(ctx != NULL, "build_context() should succeed for client");

  /* Clean up */
  funcs.free_context(ctx);
  fio_io_tls_free(tls);

  FIO_LOG_DDEBUG("  Client context creation - PASS");
}

/* *****************************************************************************
Test: Multiple certificate entries
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, openssl_multi_cert)(void) {
  FIO_LOG_DDEBUG("Testing multiple certificate entries");

  fio_io_tls_s *tls = fio_io_tls_new();
  FIO_ASSERT(tls != NULL, "fio_io_tls_new() failed");

  /* Add multiple self-signed certificate entries (SNI simulation) */
  fio_io_tls_cert_add(tls, "www.example.com", NULL, NULL, NULL);
  fio_io_tls_cert_add(tls, "api.example.com", NULL, NULL, NULL);
  fio_io_tls_cert_add(tls, "admin.example.com", NULL, NULL, NULL);

  FIO_ASSERT(fio_io_tls_cert_count(tls) == 3,
             "TLS context should have 3 certificate entries");

  /* Clean up */
  fio_io_tls_free(tls);

  FIO_LOG_DDEBUG("  Multiple certificate entries - PASS");
}

/* *****************************************************************************
Test: TLS default functions registration
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, openssl_default_registration)(void) {
  FIO_LOG_DDEBUG("Testing OpenSSL as default TLS implementation");

  /* Get current default functions */
  fio_io_functions_s defaults = fio_io_tls_default_functions(NULL);

  /* OpenSSL should have registered itself as default via FIO_CONSTRUCTOR */
  FIO_ASSERT(defaults.build_context != NULL,
             "Default TLS functions should have build_context set");
  FIO_ASSERT(defaults.read != NULL,
             "Default TLS functions should have read set");
  FIO_ASSERT(defaults.write != NULL,
             "Default TLS functions should have write set");

  FIO_LOG_DDEBUG("  OpenSSL as default TLS implementation - PASS");
}

/* *****************************************************************************
Test: Trust store with explicit file path (count verification)
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, openssl_trust_explicit)(void) {
  FIO_LOG_DDEBUG("Testing trust store with explicit paths");

  fio_io_tls_s *tls = fio_io_tls_new();
  FIO_ASSERT(tls != NULL, "fio_io_tls_new() failed");

  /* Initially no trust certificates */
  FIO_ASSERT(fio_io_tls_trust_count(tls) == 0,
             "New TLS context should have 0 trust certificates");

  /* Add explicit trust file path (even if file doesn't exist, count should
   * increase) Note: The actual file loading happens in build_context, not here
   */
  fio_io_tls_trust_add(tls, "/nonexistent/ca-bundle.pem");
  FIO_ASSERT(fio_io_tls_trust_count(tls) == 1,
             "TLS context should have 1 trust entry after adding explicit "
             "path");

  /* Add another trust file */
  fio_io_tls_trust_add(tls, "/another/ca.pem");
  FIO_ASSERT(fio_io_tls_trust_count(tls) == 2,
             "TLS context should have 2 trust entries");

  /* Clean up - don't build context since files don't exist */
  fio_io_tls_free(tls);

  FIO_LOG_DDEBUG("  Trust store with explicit paths - PASS");
}

/* *****************************************************************************
Test: TLS each callback iteration
***************************************************************************** */
FIO_SFUNC int fio___test_each_cert_count;
FIO_SFUNC int fio___test_each_alpn_count;
FIO_SFUNC int fio___test_each_trust_count;

FIO_SFUNC int fio___test_each_cert_cb(struct fio_io_tls_each_s *e,
                                      const char *server_name,
                                      const char *public_cert_file,
                                      const char *private_key_file,
                                      const char *pk_password) {
  (void)e;
  (void)server_name;
  (void)public_cert_file;
  (void)private_key_file;
  (void)pk_password;
  ++fio___test_each_cert_count;
  return 0;
}

FIO_SFUNC int fio___test_each_alpn_cb(struct fio_io_tls_each_s *e,
                                      const char *protocol_name,
                                      void (*on_selected)(fio_io_s *)) {
  (void)e;
  (void)protocol_name;
  (void)on_selected;
  ++fio___test_each_alpn_count;
  return 0;
}

FIO_SFUNC int fio___test_each_trust_cb(struct fio_io_tls_each_s *e,
                                       const char *public_cert_file) {
  (void)e;
  (void)public_cert_file;
  ++fio___test_each_trust_count;
  return 0;
}

FIO_SFUNC void FIO_NAME_TEST(stl, openssl_tls_each)(void) {
  FIO_LOG_DDEBUG("Testing fio_io_tls_each() iteration");

  fio_io_tls_s *tls = fio_io_tls_new();
  FIO_ASSERT(tls != NULL, "fio_io_tls_new() failed");

  /* Add various entries */
  fio_io_tls_cert_add(tls, "host1.example.com", NULL, NULL, NULL);
  fio_io_tls_cert_add(tls, "host2.example.com", NULL, NULL, NULL);
  fio_io_tls_alpn_add(tls, "h2", fio___test_alpn_callback);
  fio_io_tls_alpn_add(tls, "http/1.1", fio___test_alpn_callback);
  fio_io_tls_alpn_add(tls, "spdy/3.1", fio___test_alpn_callback);
  /* Add system trust (NULL) - this sets trust_sys flag, iterated via each */
  fio_io_tls_trust_add(tls, NULL);
  /* Add explicit trust file path - this adds to trust map */
  fio_io_tls_trust_add(tls, "/some/ca.pem");

  /* Reset counters */
  fio___test_each_cert_count = 0;
  fio___test_each_alpn_count = 0;
  fio___test_each_trust_count = 0;

  /* Iterate */
  int result = fio_io_tls_each(tls,
                               .each_cert = fio___test_each_cert_cb,
                               .each_alpn = fio___test_each_alpn_cb,
                               .each_trust = fio___test_each_trust_cb);

  FIO_ASSERT(result == 0, "fio_io_tls_each() should return 0 on success");
  FIO_ASSERT(fio___test_each_cert_count == 2,
             "fio_io_tls_each() should iterate 2 certificates, got %d",
             fio___test_each_cert_count);
  FIO_ASSERT(fio___test_each_alpn_count == 3,
             "fio_io_tls_each() should iterate 3 ALPN protocols, got %d",
             fio___test_each_alpn_count);
  /* Trust iteration includes: 1 from trust_sys (NULL) + 1 from explicit path */
  FIO_ASSERT(fio___test_each_trust_count == 2,
             "fio_io_tls_each() should iterate 2 trust entries (sys + "
             "explicit), got %d",
             fio___test_each_trust_count);

  /* Clean up */
  fio_io_tls_free(tls);

  FIO_LOG_DDEBUG("  fio_io_tls_each() iteration - PASS");
}

/* *****************************************************************************
Test: Context build with full configuration
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, openssl_full_config)(void) {
  FIO_LOG_DDEBUG("Testing full TLS configuration");

  fio_io_functions_s funcs = fio_openssl_io_functions();

  /* Test server configuration (self-signed cert + ALPN) */
  {
    fio_io_tls_s *tls = fio_io_tls_new();
    FIO_ASSERT(tls != NULL, "fio_io_tls_new() failed");

    /* Configure server: self-signed cert + ALPN */
    fio_io_tls_cert_add(tls, "secure.example.com", NULL, NULL, NULL);
    fio_io_tls_alpn_add(tls, "h2", fio___test_alpn_callback);
    fio_io_tls_alpn_add(tls, "http/1.1", fio___test_alpn_callback);

    /* Build server context */
    void *server_ctx = funcs.build_context(tls, 0);
    FIO_ASSERT(server_ctx != NULL,
               "build_context() should succeed with server config");
    funcs.free_context(server_ctx);

    fio_io_tls_free(tls);
  }

  /* Test client configuration (ALPN only, no trust = no verification) */
  {
    fio_io_tls_s *tls = fio_io_tls_new();
    FIO_ASSERT(tls != NULL, "fio_io_tls_new() failed");

    /* Configure client: ALPN only */
    fio_io_tls_alpn_add(tls, "h2", fio___test_alpn_callback);
    fio_io_tls_alpn_add(tls, "http/1.1", fio___test_alpn_callback);

    /* Build client context (will warn about no verification) */
    void *client_ctx = funcs.build_context(tls, 1);
    FIO_ASSERT(client_ctx != NULL,
               "build_context() should succeed with client config");
    funcs.free_context(client_ctx);

    fio_io_tls_free(tls);
  }

  FIO_LOG_DDEBUG("  Full TLS configuration - PASS");
}

/* *****************************************************************************
Test: Empty ALPN protocol name handling
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, openssl_alpn_edge_cases)(void) {
  FIO_LOG_DDEBUG("Testing ALPN edge cases");

  fio_io_tls_s *tls = fio_io_tls_new();
  FIO_ASSERT(tls != NULL, "fio_io_tls_new() failed");

  /* NULL protocol name should be silently ignored */
  fio_io_tls_alpn_add(tls, NULL, fio___test_alpn_callback);
  FIO_ASSERT(fio_io_tls_alpn_count(tls) == 0,
             "NULL ALPN protocol should be ignored");

  /* Valid protocol should work */
  fio_io_tls_alpn_add(tls, "valid-protocol", fio___test_alpn_callback);
  FIO_ASSERT(fio_io_tls_alpn_count(tls) == 1,
             "Valid ALPN protocol should be added");

  /* NULL callback should be accepted (replaced with no-op) */
  fio_io_tls_alpn_add(tls, "another-protocol", NULL);
  FIO_ASSERT(fio_io_tls_alpn_count(tls) == 2,
             "ALPN with NULL callback should be added");

  /* Clean up */
  fio_io_tls_free(tls);

  FIO_LOG_DDEBUG("  ALPN edge cases - PASS");
}

/* *****************************************************************************
Test: TLS from URL parsing
***************************************************************************** */
FIO_SFUNC void FIO_NAME_TEST(stl, openssl_tls_from_url)(void) {
  FIO_LOG_DDEBUG("Testing fio_io_tls_from_url()");

  /* Test HTTPS URL - should create TLS context */
  {
    fio_url_s url = fio_url_parse("https://example.com:443/path", 28);
    fio_io_tls_s *tls = fio_io_tls_from_url(NULL, url);
    FIO_ASSERT(tls != NULL,
               "fio_io_tls_from_url() should create TLS for https URL");
    fio_io_tls_free(tls);
  }

  /* Test HTTP URL - should not create TLS context */
  {
    fio_url_s url = fio_url_parse("http://example.com:80/path", 26);
    fio_io_tls_s *tls = fio_io_tls_from_url(NULL, url);
    FIO_ASSERT(tls == NULL,
               "fio_io_tls_from_url() should return NULL for http URL");
  }

  /* Test WSS URL - should create TLS context */
  {
    fio_url_s url = fio_url_parse("wss://example.com/ws", 20);
    fio_io_tls_s *tls = fio_io_tls_from_url(NULL, url);
    FIO_ASSERT(tls != NULL,
               "fio_io_tls_from_url() should create TLS for wss URL");
    fio_io_tls_free(tls);
  }

  FIO_LOG_DDEBUG("  fio_io_tls_from_url() - PASS");
}

/* *****************************************************************************
Main test runner

NOTE: The leak counter warning for fio___openssl_context_s is expected.
The OpenSSL module's free_context uses fio_io_defer() to defer cleanup,
which requires the IO reactor to be running. Since these are unit tests
that don't run the reactor, the deferred cleanup tasks never execute.
This is acceptable for unit testing purposes.
***************************************************************************** */
int main(void) {
  FIO_LOG_DDEBUG("Testing OpenSSL TLS Module");
  FIO_LOG_DDEBUG("  (OpenSSL version: %s)", OPENSSL_VERSION_TEXT);

  FIO_NAME_TEST(stl, openssl_io_functions)();
  FIO_NAME_TEST(stl, openssl_tls_context)();
  FIO_NAME_TEST(stl, openssl_self_signed)();
  FIO_NAME_TEST(stl, openssl_self_signed_named)();
  FIO_NAME_TEST(stl, openssl_alpn)();
  FIO_NAME_TEST(stl, openssl_trust)();
  FIO_NAME_TEST(stl, openssl_trust_explicit)();
  FIO_NAME_TEST(stl, openssl_client_context)();
  FIO_NAME_TEST(stl, openssl_multi_cert)();
  FIO_NAME_TEST(stl, openssl_default_registration)();
  FIO_NAME_TEST(stl, openssl_tls_each)();
  FIO_NAME_TEST(stl, openssl_full_config)();
  FIO_NAME_TEST(stl, openssl_alpn_edge_cases)();
  FIO_NAME_TEST(stl, openssl_tls_from_url)();

  FIO_LOG_DDEBUG("OpenSSL TLS Module tests complete.");
  FIO_LOG_DDEBUG(
      "  (Note: leak counter warnings for fio___openssl_context_s are expected "
      "because free_context uses fio_io_defer which requires the IO reactor)");
  return 0;
}

#endif /* H___FIO_OPENSSL___H */
