/* *****************************************************************************
TLS 1.3 CertificateVerify signature verification test.
***************************************************************************** */
#define FIO_LOG
#define FIO_TEST_CSTL
#define FIO_CRYPT
#define FIO_TLS13
#define FIO_X509
#include "test-helpers.h"

FIO_SFUNC void fio___test_tls13_cv_p256(void) {
  fio_x509_keypair_s keypair;
  FIO_ASSERT(fio_x509_keypair_p256(&keypair) == 0,
             "P-256 keypair generation should succeed");

  const char *san_dns[] = {"localhost"};
  fio_x509_cert_options_s opts = {
      .subject_cn = "localhost",
      .subject_cn_len = 9,
      .san_dns = san_dns,
      .san_dns_count = 1,
      .is_ca = 0,
  };

  size_t cert_size = fio_x509_self_signed_cert(NULL, 0, &keypair, &opts);
  FIO_ASSERT(cert_size > 0, "cert size calculation should succeed");

  uint8_t *cert = (uint8_t *)FIO_MEM_REALLOC(NULL, 0, cert_size + 100, 0);
  FIO_ASSERT(cert, "cert allocation should succeed");

  size_t cert_len =
      fio_x509_self_signed_cert(cert, cert_size + 100, &keypair, &opts);
  FIO_ASSERT(cert_len > 0, "cert generation should succeed");

  fio_tls13_server_s server;
  fio_tls13_server_init(&server);
  const uint8_t *certs[] = {cert};
  size_t cert_lens[] = {cert_len};
  fio_tls13_server_set_cert_chain(&server, certs, cert_lens, 1);
  fio_tls13_server_set_private_key(
      &server, keypair.secret_key, 32, FIO_TLS13_SIG_ECDSA_SECP256R1_SHA256);
  FIO_MEMCPY(server.public_key, keypair.public_key, 65);

  fio_tls13_client_s client;
  fio_tls13_client_init(&client, "localhost");

  /* Build a trust store containing the self-signed certificate. */
  const uint8_t *trust_certs[] = {cert};
  size_t trust_lens[] = {cert_len};
  fio_x509_trust_store_s trust = {
      .roots = trust_certs,
      .root_lens = trust_lens,
      .root_count = 1,
  };
  fio_tls13_client_set_trust_store(&client, &trust);

  uint8_t client_out[8192];
  uint8_t server_out[8192];
  size_t out_len;

  int ch_len = fio_tls13_client_start(&client, client_out, sizeof(client_out));
  FIO_ASSERT(ch_len > 0, "ClientHello generation should succeed");

  int consumed = fio_tls13_server_process(&server,
                                          client_out,
                                          (size_t)ch_len,
                                          server_out,
                                          sizeof(server_out),
                                          &out_len);
  FIO_ASSERT(consumed > 0, "Server should process ClientHello");
  FIO_ASSERT(out_len > 0, "Server should generate response");

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

  if (!fio_tls13_client_is_connected(&client)) {
    fprintf(stderr,
            "client not connected: alert=%u (%s) cert_error=%d\n",
            client.alert_description,
            fio_tls13_alert_name(client.alert_description),
            client.cert_error);
  }
  FIO_ASSERT(fio_tls13_client_is_connected(&client),
             "Client should verify server CertificateVerify and chain");

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

  fio_tls13_client_destroy(&client);
  fio_tls13_server_destroy(&server);
  fio_x509_keypair_clear(&keypair);
  FIO_MEM_FREE(cert, cert_size + 100);
}

int main(void) {
  fio___test_tls13_cv_p256();
  return 0;
}
