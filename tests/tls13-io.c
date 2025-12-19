/* *****************************************************************************
TLS 1.3 IO Functions Test

Tests the fio_io_functions_s interface for TLS 1.3.
***************************************************************************** */

/* Include all required modules */
#define FIO_LOG
#define FIO_TEST_CSTL
#define FIO_CRYPT
#define FIO_TLS13
#define FIO_X509
#define FIO_IO
#include "fio-stl.h"

/* *****************************************************************************
Test Helpers
***************************************************************************** */

FIO_SFUNC void fio___test_tls13_io_print_hex(const char *label,
                                             const uint8_t *data,
                                             size_t len) {
  fprintf(stderr, "%s (%zu bytes): ", label, len);
  size_t print_len = (len > 32) ? 32 : len;
  for (size_t i = 0; i < print_len; ++i)
    fprintf(stderr, "%02X", data[i]);
  if (len > 32)
    fprintf(stderr, "...");
  fprintf(stderr, "\n");
}

/* *****************************************************************************
Test: Context Creation and Destruction
***************************************************************************** */

FIO_SFUNC void fio___test_tls13_io_context(void) {
  FIO_LOG_DDEBUG("Testing TLS 1.3 IO context creation/destruction\n");

  /* Get TLS 1.3 IO functions */
  fio_io_functions_s funcs = fio_tls13_io_functions();
  FIO_ASSERT(funcs.build_context, "build_context should not be NULL");
  FIO_ASSERT(funcs.free_context, "free_context should not be NULL");
  FIO_ASSERT(funcs.start, "start should not be NULL");
  FIO_ASSERT(funcs.read, "read should not be NULL");
  FIO_ASSERT(funcs.write, "write should not be NULL");
  FIO_ASSERT(funcs.flush, "flush should not be NULL");
  FIO_ASSERT(funcs.cleanup, "cleanup should not be NULL");
  FIO_ASSERT(funcs.finish, "finish should not be NULL");

  /* Create TLS configuration */
  fio_io_tls_s *tls = fio_io_tls_new();
  FIO_ASSERT(tls, "fio_io_tls_new should return non-NULL");

  /* Build server context (will generate self-signed cert) */
  void *server_ctx = funcs.build_context(tls, 0);
  FIO_ASSERT(server_ctx, "build_context for server should return non-NULL");

  /* Build client context */
  void *client_ctx = funcs.build_context(tls, 1);
  FIO_ASSERT(client_ctx, "build_context for client should return non-NULL");

  /* Free contexts */
  funcs.free_context(server_ctx);
  funcs.free_context(client_ctx);

  /* Free TLS configuration */
  fio_io_tls_free(tls);

  FIO_LOG_DDEBUG("  - Context creation/destruction: PASSED\n");
}

/* *****************************************************************************
Test: Self-Signed Certificate Generation
***************************************************************************** */

FIO_SFUNC void fio___test_tls13_io_self_signed(void) {
  FIO_LOG_DDEBUG("Testing TLS 1.3 self-signed certificate generation\n");

  /* Get TLS 1.3 IO functions */
  fio_io_functions_s funcs = fio_tls13_io_functions();

  /* Create TLS configuration without certificates */
  fio_io_tls_s *tls = fio_io_tls_new();
  FIO_ASSERT(tls, "fio_io_tls_new should return non-NULL");

  /* Build server context - should generate self-signed cert */
  void *server_ctx = funcs.build_context(tls, 0);
  FIO_ASSERT(server_ctx, "build_context should generate self-signed cert");

  /* Free context */
  funcs.free_context(server_ctx);
  fio_io_tls_free(tls);

  FIO_LOG_DDEBUG("  - Self-signed certificate generation: PASSED\n");
}

/* *****************************************************************************
Test: IO Functions Structure
***************************************************************************** */

FIO_SFUNC void fio___test_tls13_io_functions_struct(void) {
  FIO_LOG_DDEBUG("Testing TLS 1.3 IO functions structure\n");

  fio_io_functions_s funcs = fio_tls13_io_functions();

  /* Verify all function pointers are set */
  FIO_ASSERT(funcs.build_context != NULL, "build_context must be set");
  FIO_ASSERT(funcs.free_context != NULL, "free_context must be set");
  FIO_ASSERT(funcs.start != NULL, "start must be set");
  FIO_ASSERT(funcs.read != NULL, "read must be set");
  FIO_ASSERT(funcs.write != NULL, "write must be set");
  FIO_ASSERT(funcs.flush != NULL, "flush must be set");
  FIO_ASSERT(funcs.cleanup != NULL, "cleanup must be set");
  FIO_ASSERT(funcs.finish != NULL, "finish must be set");

  FIO_LOG_DDEBUG("  - IO functions structure: PASSED\n");
}

/* *****************************************************************************
Test: Default TLS Functions Registration
***************************************************************************** */

FIO_SFUNC void fio___test_tls13_io_default_registration(void) {
  FIO_LOG_DDEBUG("Testing TLS 1.3 default registration\n");

  /* Get current default functions */
  fio_io_functions_s defaults = fio_io_tls_default_functions(NULL);

  /* Verify defaults are set (either OpenSSL or TLS 1.3) */
  FIO_ASSERT(defaults.build_context != NULL,
             "default build_context should be set");
  FIO_ASSERT(defaults.read != NULL, "default read should be set");
  FIO_ASSERT(defaults.write != NULL, "default write should be set");

  FIO_LOG_DDEBUG("  - Default TLS functions registration: PASSED\n");
}

/* *****************************************************************************
Test: TLS Configuration with Certificates
***************************************************************************** */

FIO_SFUNC void fio___test_tls13_io_tls_config(void) {
  FIO_LOG_DDEBUG("Testing TLS 1.3 with fio_io_tls_s configuration\n");

  /* Create TLS configuration */
  fio_io_tls_s *tls = fio_io_tls_new();
  FIO_ASSERT(tls, "fio_io_tls_new should return non-NULL");

  /* Add certificate (will use self-signed since files don't exist) */
  fio_io_tls_cert_add(tls, "localhost", NULL, NULL, NULL);

  /* Verify certificate count */
  FIO_ASSERT(fio_io_tls_cert_count(tls) == 1,
             "cert_count should be 1 after adding cert");

  /* Build context */
  fio_io_functions_s funcs = fio_tls13_io_functions();
  void *ctx = funcs.build_context(tls, 0);
  FIO_ASSERT(ctx, "build_context should succeed with cert config");

  /* Cleanup */
  funcs.free_context(ctx);
  fio_io_tls_free(tls);

  FIO_LOG_DDEBUG("  - TLS configuration: PASSED\n");
}

/* *****************************************************************************
Test: Multiple Context Creation
***************************************************************************** */

FIO_SFUNC void fio___test_tls13_io_multiple_contexts(void) {
  FIO_LOG_DDEBUG("Testing TLS 1.3 multiple context creation\n");

  fio_io_functions_s funcs = fio_tls13_io_functions();
  fio_io_tls_s *tls = fio_io_tls_new();

  /* Create multiple server contexts */
  void *ctx1 = funcs.build_context(tls, 0);
  void *ctx2 = funcs.build_context(tls, 0);
  void *ctx3 = funcs.build_context(tls, 0);

  FIO_ASSERT(ctx1 && ctx2 && ctx3, "all contexts should be created");
  FIO_ASSERT(ctx1 != ctx2 && ctx2 != ctx3, "contexts should be unique");

  /* Create client contexts */
  void *client1 = funcs.build_context(tls, 1);
  void *client2 = funcs.build_context(tls, 1);

  FIO_ASSERT(client1 && client2, "client contexts should be created");

  /* Cleanup */
  funcs.free_context(ctx1);
  funcs.free_context(ctx2);
  funcs.free_context(ctx3);
  funcs.free_context(client1);
  funcs.free_context(client2);
  fio_io_tls_free(tls);

  FIO_LOG_DDEBUG("  - Multiple context creation: PASSED\n");
}

/* *****************************************************************************
Test: Underlying TLS 1.3 Client/Server Handshake (Unit Test)
***************************************************************************** */

FIO_SFUNC void fio___test_tls13_io_handshake_unit(void) {
  FIO_LOG_DDEBUG("Testing TLS 1.3 client/server handshake (unit test)\n");

  /* Generate self-signed certificate for server */
  fio_x509_keypair_s keypair;
  FIO_ASSERT(fio_x509_keypair_ed25519(&keypair) == 0,
             "keypair generation should succeed");

  fio_x509_cert_options_s opts = {
      .subject_cn = "localhost",
      .subject_cn_len = 9,
  };

  /* Calculate cert size */
  size_t cert_size = fio_x509_self_signed_cert(NULL, 0, &keypair, &opts);
  FIO_ASSERT(cert_size > 0, "cert size calculation should succeed");

  /* Generate certificate */
  uint8_t *cert = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, cert_size, 0);
  FIO_ASSERT(cert, "cert allocation should succeed");

  size_t cert_len = fio_x509_self_signed_cert(cert, cert_size, &keypair, &opts);
  FIO_ASSERT(cert_len > 0, "cert generation should succeed");

  /* Initialize server */
  fio_tls13_server_s server;
  fio_tls13_server_init(&server);

  const uint8_t *certs[] = {cert};
  size_t cert_lens[] = {cert_len};
  fio_tls13_server_set_cert_chain(&server, certs, cert_lens, 1);
  fio_tls13_server_set_private_key(&server,
                                   keypair.secret_key,
                                   32,
                                   FIO_TLS13_SIG_ED25519);

  /* Initialize client */
  fio_tls13_client_s client;
  fio_tls13_client_init(&client, "localhost");
  fio_tls13_client_skip_verification(&client, 1); /* Skip for unit test */

  /* Buffers for handshake */
  uint8_t client_out[8192];
  uint8_t server_out[8192];
  size_t out_len;

  /* Client sends ClientHello */
  int ch_len = fio_tls13_client_start(&client, client_out, sizeof(client_out));
  FIO_ASSERT(ch_len > 0, "ClientHello generation should succeed");
  FIO_LOG_DDEBUG("  - ClientHello: %d bytes\n", ch_len);

  /* Server processes ClientHello and sends response */
  int consumed = fio_tls13_server_process(&server,
                                          client_out,
                                          (size_t)ch_len,
                                          server_out,
                                          sizeof(server_out),
                                          &out_len);
  FIO_ASSERT(consumed > 0, "Server should process ClientHello");
  FIO_ASSERT(out_len > 0, "Server should generate response");
  FIO_LOG_DDEBUG("  - Server response: %zu bytes\n", out_len);

  /* Client processes server response */
  size_t client_response_len = 0;
  size_t offset = 0;
  while (offset < out_len) {
    size_t msg_out_len = 0;
    consumed = fio_tls13_client_process(&client,
                                        server_out + offset,
                                        out_len - offset,
                                        client_out,
                                        sizeof(client_out),
                                        &msg_out_len);
    if (consumed <= 0)
      break;
    offset += (size_t)consumed;
    client_response_len += msg_out_len;
  }

  FIO_ASSERT(fio_tls13_client_is_connected(&client),
             "Client should be connected after handshake");
  FIO_LOG_DDEBUG("  - Client connected: YES\n");

  /* Server processes client Finished */
  if (client_response_len > 0) {
    consumed = fio_tls13_server_process(&server,
                                        client_out,
                                        client_response_len,
                                        server_out,
                                        sizeof(server_out),
                                        &out_len);
    FIO_ASSERT(consumed > 0, "Server should process client Finished");
  }

  FIO_ASSERT(fio_tls13_server_is_connected(&server),
             "Server should be connected after handshake");
  FIO_LOG_DDEBUG("  - Server connected: YES\n");

  /* Test application data exchange */
  const char *test_msg = "Hello, TLS 1.3!";
  size_t test_msg_len = FIO_STRLEN(test_msg);

  /* Client encrypts message */
  int enc_len = fio_tls13_client_encrypt(&client,
                                         client_out,
                                         sizeof(client_out),
                                         (const uint8_t *)test_msg,
                                         test_msg_len);
  FIO_ASSERT(enc_len > 0, "Client encryption should succeed");
  FIO_LOG_DDEBUG("  - Encrypted message: %d bytes\n", enc_len);

  /* Server decrypts message */
  uint8_t decrypted[256];
  int dec_len = fio_tls13_server_decrypt(&server,
                                         decrypted,
                                         sizeof(decrypted),
                                         client_out,
                                         (size_t)enc_len);
  FIO_ASSERT(dec_len == (int)test_msg_len, "Decrypted length should match");
  FIO_ASSERT(FIO_MEMCMP(decrypted, test_msg, test_msg_len) == 0,
             "Decrypted message should match original");
  FIO_LOG_DDEBUG("  - Decrypted message: '%.*s'\n", dec_len, decrypted);

  /* Cleanup */
  fio_tls13_client_destroy(&client);
  fio_tls13_server_destroy(&server);
  fio_x509_keypair_clear(&keypair);
  FIO_MEM_FREE(cert, cert_size);

  FIO_LOG_DDEBUG("  - Client/server handshake unit test: PASSED\n");
}

/* *****************************************************************************
Main Test Function
***************************************************************************** */

void fio___test_tls13_io(void) {
  FIO_LOG_DDEBUG("=== Testing TLS 1.3 IO Functions Interface ===\n\n");

  fio___test_tls13_io_functions_struct();
  fio___test_tls13_io_context();
  fio___test_tls13_io_self_signed();
  fio___test_tls13_io_default_registration();
  fio___test_tls13_io_tls_config();
  fio___test_tls13_io_multiple_contexts();
  fio___test_tls13_io_handshake_unit();

  /* Run deferred tasks to clean up contexts */
  fio_queue_perform_all(fio_io_queue());

  FIO_LOG_DDEBUG("=== TLS 1.3 IO Functions Tests Complete ===\n\n");
}

/* *****************************************************************************
Main Entry Point
***************************************************************************** */

int main(int argc, char const *argv[]) {
  (void)argc;
  (void)argv;

  fio___test_tls13_io();

  return 0;
}
