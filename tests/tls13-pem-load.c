/* *****************************************************************************
TLS 1.3 PEM certificate/key loading test.

Verifies that the RSA ./cert.pem and ./key.pem files can be loaded by the
embedded TLS context builder.
***************************************************************************** */
#define FIO_LOG
#define FIO_TEST_CSTL
#define FIO_CRYPT
#define FIO_TLS13
#define FIO_X509
#define FIO_PEM
#define FIO_IO
#include "test-helpers.h"

FIO_SFUNC void fio___test_tls13_pem_load(void) {
  fio_io_tls_s *tls = fio_io_tls_new();
  FIO_ASSERT(tls, "fio_io_tls_new should succeed");

  fio_io_tls_cert_add(tls, "localhost", "./cert.pem", "./key.pem", NULL);

  fio_io_functions_s funcs = fio_tls13_io_functions();
  fio___tls13_context_s *ctx =
      (fio___tls13_context_s *)funcs.build_context(tls, 0);
  FIO_ASSERT(ctx, "build_context should load RSA cert/key");
  FIO_ASSERT(ctx->cert_chain_count >= 1,
             "certificate chain should be loaded");
  FIO_ASSERT(ctx->private_key_type == FIO_TLS13_SIG_RSA_PSS_RSAE_SHA256,
             "RSA private key should be selected");
  FIO_ASSERT(ctx->private_key_ext != NULL,
             "RSA private key buffer should be allocated");
  FIO_ASSERT(ctx->private_key_ext_len > 0,
             "RSA private key buffer length should be > 0");

  funcs.free_context(ctx);
  fio_io_tls_free(tls);
  fio_queue_perform_all(fio_io_queue());
}

int main(void) {
  fio___test_tls13_pem_load();
  return 0;
}
