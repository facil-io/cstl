#define FIO_LOG
#define FIO_CRYPT
#define FIO_TLS13
#include "test-helpers.h"

static const uint8_t test_ed25519_private_key[32] = {
    0x9d, 0x61, 0xb1, 0x9d, 0xef, 0xfd, 0x5a, 0x60, 0xba, 0x84, 0x4a,
    0xf4, 0x92, 0xec, 0x2c, 0xc4, 0x44, 0x49, 0xc5, 0x69, 0x7b, 0x32,
    0x69, 0x19, 0x70, 0x3b, 0xac, 0x03, 0x1c, 0xae, 0x7f, 0x60};

static uint8_t test_ed25519_public_key[32];
static uint8_t test_certificate[512];
static size_t test_certificate_len = 0;

FIO_SFUNC void build_test_certificate(void) {
  fio_ed25519_public_key(test_ed25519_public_key, test_ed25519_private_key);
  uint8_t *p = test_certificate;
  *p++ = 0x30;
  uint8_t *cert_len_ptr = p++;
  uint8_t *cert_start = p;
  *p++ = 0x30;
  uint8_t *tbs_len_ptr = p++;
  uint8_t *tbs_start = p;
  *p++ = 0xA0;
  *p++ = 0x03;
  *p++ = 0x02;
  *p++ = 0x01;
  *p++ = 0x02;
  *p++ = 0x02;
  *p++ = 0x01;
  *p++ = 0x01;
  *p++ = 0x30;
  *p++ = 0x05;
  *p++ = 0x06;
  *p++ = 0x03;
  *p++ = 0x2B;
  *p++ = 0x65;
  *p++ = 0x70;
  *p++ = 0x30;
  *p++ = 0x0F;
  *p++ = 0x31;
  *p++ = 0x0D;
  *p++ = 0x30;
  *p++ = 0x0B;
  *p++ = 0x06;
  *p++ = 0x03;
  *p++ = 0x55;
  *p++ = 0x04;
  *p++ = 0x03;
  *p++ = 0x0C;
  *p++ = 0x04;
  *p++ = 't';
  *p++ = 'e';
  *p++ = 's';
  *p++ = 't';
  *p++ = 0x30;
  *p++ = 0x1E;
  *p++ = 0x17;
  *p++ = 0x0D;
  FIO_MEMCPY(p, "240101000000Z", 13);
  p += 13;
  *p++ = 0x17;
  *p++ = 0x0D;
  FIO_MEMCPY(p, "341231235959Z", 13);
  p += 13;
  *p++ = 0x30;
  *p++ = 0x0F;
  *p++ = 0x31;
  *p++ = 0x0D;
  *p++ = 0x30;
  *p++ = 0x0B;
  *p++ = 0x06;
  *p++ = 0x03;
  *p++ = 0x55;
  *p++ = 0x04;
  *p++ = 0x03;
  *p++ = 0x0C;
  *p++ = 0x04;
  *p++ = 't';
  *p++ = 'e';
  *p++ = 's';
  *p++ = 't';
  *p++ = 0x30;
  *p++ = 0x2A;
  *p++ = 0x30;
  *p++ = 0x05;
  *p++ = 0x06;
  *p++ = 0x03;
  *p++ = 0x2B;
  *p++ = 0x65;
  *p++ = 0x70;
  *p++ = 0x03;
  *p++ = 0x21;
  *p++ = 0x00;
  FIO_MEMCPY(p, test_ed25519_public_key, 32);
  p += 32;
  *tbs_len_ptr = (uint8_t)(p - tbs_start);
  *p++ = 0x30;
  *p++ = 0x05;
  *p++ = 0x06;
  *p++ = 0x03;
  *p++ = 0x2B;
  *p++ = 0x65;
  *p++ = 0x70;
  uint8_t signature[64];
  fio_ed25519_sign(signature,
                   tbs_start - 2,
                   (size_t)(p - tbs_start + 2),
                   test_ed25519_private_key,
                   test_ed25519_public_key);
  *p++ = 0x03;
  *p++ = 0x41;
  *p++ = 0x00;
  FIO_MEMCPY(p, signature, 64);
  p += 64;
  *cert_len_ptr = (uint8_t)(p - cert_start);
  test_certificate_len = (size_t)(p - test_certificate);
}

int main(void) {
  build_test_certificate();

  fio_tls13_server_s server;
  fio_tls13_server_init(&server);
  const uint8_t *certs[] = {test_certificate};
  size_t cert_lens[] = {test_certificate_len};
  fio_tls13_server_set_cert_chain(&server, certs, cert_lens, 1);
  fio_tls13_server_set_private_key(&server,
                                   test_ed25519_private_key,
                                   32,
                                   FIO_TLS13_SIG_ED25519);

  fio_tls13_client_s client;
  fio_tls13_client_init(&client, "test");
  fio_tls13_client_skip_verification(&client, 1);

  uint8_t client_out[8192];
  uint8_t server_out[8192];
  size_t out_len;

  fprintf(stderr, "Step 1: Client generates ClientHello\n");
  int ch_len = fio_tls13_client_start(&client, client_out, sizeof(client_out));
  fprintf(stderr,
          "  ch_len=%d, client state=%s\n",
          ch_len,
          fio_tls13_client_state_name(&client));

  fprintf(stderr, "Step 2: Server processes ClientHello\n");
  out_len = 0;
  int consumed = fio_tls13_server_process(&server,
                                          client_out,
                                          (size_t)ch_len,
                                          server_out,
                                          sizeof(server_out),
                                          &out_len);
  fprintf(stderr,
          "  consumed=%d, out_len=%zu, server state=%s\n",
          consumed,
          out_len,
          fio_tls13_server_state_name(&server));

  fprintf(stderr, "Step 3: Client processes server response\n");
  size_t client_response_len = 0;
  size_t offset = 0;
  int iteration = 0;
  while (offset < out_len && !fio_tls13_client_is_connected(&client) &&
         !fio_tls13_client_is_error(&client)) {
    size_t msg_out_len = 0;
    fprintf(stderr,
            "  Iteration %d: offset=%zu, remaining=%zu, client state=%s\n",
            iteration,
            offset,
            out_len - offset,
            fio_tls13_client_state_name(&client));
    int proc =
        fio_tls13_client_process(&client,
                                 server_out + offset,
                                 out_len - offset,
                                 client_out + client_response_len,
                                 sizeof(client_out) - client_response_len,
                                 &msg_out_len);
    fprintf(stderr,
            "    proc=%d, msg_out_len=%zu, new state=%s, error=%d, alert=%d\n",
            proc,
            msg_out_len,
            fio_tls13_client_state_name(&client),
            fio_tls13_client_is_error(&client),
            client.alert_description);
    if (proc <= 0) {
      fprintf(stderr, "    Breaking due to proc <= 0\n");
      break;
    }
    offset += (size_t)proc;
    client_response_len += msg_out_len;
    iteration++;
  }

  fprintf(stderr,
          "After loop: client state=%s, error=%d, alert=%d\n",
          fio_tls13_client_state_name(&client),
          fio_tls13_client_is_error(&client),
          client.alert_description);
  fprintf(stderr, "  client_response_len=%zu\n", client_response_len);

  if (fio_tls13_client_is_error(&client)) {
    fprintf(stderr, "ERROR: Client is in error state!\n");
    fio_tls13_client_destroy(&client);
    fio_tls13_server_destroy(&server);
    return 1;
  }

  fio_tls13_client_destroy(&client);
  fio_tls13_server_destroy(&server);

  fprintf(stderr, "Test passed!\n");
  return 0;
}
