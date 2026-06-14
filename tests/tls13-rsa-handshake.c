/* *****************************************************************************
TLS 1.3 RSA certificate handshake test.
***************************************************************************** */
#define FIO_LOG
#define FIO_TEST_CSTL
#define FIO_CRYPT
#define FIO_TLS13
#define FIO_X509
#define FIO_PEM
#include "test-helpers.h"

int main(void) {
  char *cert_pem = fio_bstr_readfile(NULL, "./cert.pem", 0, 0);
  char *key_pem = fio_bstr_readfile(NULL, "./key.pem", 0, 0);
  FIO_ASSERT(cert_pem, "cert.pem should exist");
  FIO_ASSERT(key_pem, "key.pem should exist");

  size_t cert_pem_len = fio_bstr_len(cert_pem);
  size_t key_pem_len = fio_bstr_len(key_pem);

  uint8_t cert[8192];
  size_t cert_len =
      fio_pem_get_certificate_der(cert, sizeof(cert), cert_pem, cert_pem_len);
  FIO_ASSERT(cert_len > 0, "certificate DER extraction should succeed");

  fio_pem_private_key_s pkey;
  FIO_ASSERT(fio_pem_parse_private_key(&pkey, key_pem, key_pem_len) == 0,
             "private key parsing should succeed");
  FIO_ASSERT(pkey.type == FIO_PEM_KEY_RSA, "key should be RSA");

  FIO_ASSERT(pkey.rsa.e_len && pkey.rsa.p_len && pkey.rsa.q_len &&
                 pkey.rsa.dP_len && pkey.rsa.dQ_len && pkey.rsa.qInv_len,
             "parsed RSA key should contain CRT parameters");

  uint8_t key_buf[4096];
  size_t key_buf_len = 0;

#define APPEND_RSA_FIELD(field, len)                                           \
  do {                                                                         \
    FIO_ASSERT(key_buf_len + 4 + (len) <= sizeof(key_buf),                    \
               "key buffer large enough for RSA field");                       \
    key_buf[key_buf_len++] = (uint8_t)((len) >> 24);                           \
    key_buf[key_buf_len++] = (uint8_t)((len) >> 16);                           \
    key_buf[key_buf_len++] = (uint8_t)((len) >> 8);                            \
    key_buf[key_buf_len++] = (uint8_t)((len));                                 \
    FIO_MEMCPY(key_buf + key_buf_len, (field), (len));                         \
    key_buf_len += (len);                                                      \
  } while (0)

  APPEND_RSA_FIELD(pkey.rsa.n, pkey.rsa.n_len);
  APPEND_RSA_FIELD(pkey.rsa.d, pkey.rsa.d_len);
  APPEND_RSA_FIELD(pkey.rsa.e, pkey.rsa.e_len);
  APPEND_RSA_FIELD(pkey.rsa.p, pkey.rsa.p_len);
  APPEND_RSA_FIELD(pkey.rsa.q, pkey.rsa.q_len);
  APPEND_RSA_FIELD(pkey.rsa.dP, pkey.rsa.dP_len);
  APPEND_RSA_FIELD(pkey.rsa.dQ, pkey.rsa.dQ_len);
  APPEND_RSA_FIELD(pkey.rsa.qInv, pkey.rsa.qInv_len);

#undef APPEND_RSA_FIELD

  fio_tls13_server_s server;
  fio_tls13_server_init(&server);
  const uint8_t *certs[] = {cert};
  size_t cert_lens[] = {cert_len};
  fio_tls13_server_set_cert_chain(&server, certs, cert_lens, 1);
  fio_tls13_server_set_private_key(
      &server, key_buf, key_buf_len, FIO_TLS13_SIG_RSA_PSS_RSAE_SHA256);

  fio_tls13_client_s client;
  fio_tls13_client_init(&client, "localhost");
  fio_tls13_client_skip_verification(&client, 1);

  uint8_t client_out[8192];
  uint8_t server_out[8192];
  size_t out_len;

  int ch_len =
      fio_tls13_client_start(&client, client_out, sizeof(client_out));
  FIO_ASSERT(ch_len > 0, "ClientHello generation should succeed");

  int consumed = fio_tls13_server_process(
      &server, client_out, (size_t)ch_len, server_out, sizeof(server_out), &out_len);
  FIO_ASSERT(consumed > 0, "Server should process ClientHello");
  FIO_ASSERT(out_len > 0, "Server should generate response");

  size_t client_response_len = 0;
  size_t offset = 0;
  while (offset < out_len) {
    size_t msg_out_len = 0;
    consumed = fio_tls13_client_process(
        &client,
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
             "Client should be connected after RSA handshake");

  if (client_response_len > 0) {
    consumed = fio_tls13_server_process(
        &server,
        client_out,
        client_response_len,
        server_out,
        sizeof(server_out),
        &out_len);
    FIO_ASSERT(consumed > 0, "Server should process client Finished");
  }

  FIO_ASSERT(fio_tls13_server_is_connected(&server),
             "Server should be connected after RSA handshake");

  fio_tls13_client_destroy(&client);
  fio_tls13_server_destroy(&server);
  fio_pem_private_key_clear(&pkey);
  fio_bstr_free(cert_pem);
  fio_bstr_free(key_pem);
  return 0;
}
