/* *****************************************************************************
Test for fio-stl/190 tls13.h and fio-stl/405 tls13.h — TLS 1.3 correctness

Coverage: key schedule derivation, handshake message parsing/building,
client/server in-memory handshake roundtrip, ALPN negotiation, client
certificate authentication, KeyUpdate key rotation, large application-data
transfer, and TLS I/O function registration.  Crypto algorithm vectors are
intentionally omitted (those live in sha.c, aes.c, chacha.c, rsa.c, x509.c,
ed25519.c, p256.c, p384.c, hkdf.c).
***************************************************************************** */
#include "test-helpers.h"

#define FIO_SHA2
#define FIO_HKDF
#define FIO_AES
#define FIO_CHACHA
#define FIO_ED25519
#define FIO_P256
#define FIO_RSA
#define FIO_X509
#define FIO_IO
#define FIO_TLS13
#include FIO_INCLUDE_FILE

/* *****************************************************************************
Helpers: memory allocation and P-256 certificate generation
***************************************************************************** */
FIO_SFUNC void *tls13_test_alloc(size_t len) {
  return FIO_MEM_REALLOC(NULL, 0, len, 0);
}

FIO_SFUNC void tls13_test_free(void *p, size_t len) {
  (void)len;
  if (p)
    FIO_MEM_FREE(p, len);
}

FIO_SFUNC int tls13_test_make_p256_cert(uint8_t **der,
                                        size_t *der_len,
                                        fio_x509_keypair_s *kp_out) {
  fio_x509_keypair_s kp;
  if (fio_x509_keypair_p256(&kp) != 0)
    return -1;

  const char *san[] = {"localhost"};
  fio_x509_cert_options_s opts = {
      .subject_cn = "localhost",
      .subject_cn_len = 9,
      .subject_org = "facil.io",
      .subject_org_len = 8,
      .san_dns = san,
      .san_dns_count = 1,
      .is_ca = 0,
  };

  size_t sz = fio_x509_self_signed_cert(NULL, 0, &kp, &opts);
  if (sz == 0) {
    fio_x509_keypair_clear(&kp);
    return -1;
  }

  uint8_t *buf = (uint8_t *)tls13_test_alloc(sz);
  if (!buf) {
    fio_x509_keypair_clear(&kp);
    return -1;
  }

  size_t written = fio_x509_self_signed_cert(buf, sz, &kp, &opts);
  if (written == 0 || written > sz) {
    tls13_test_free(buf, sz);
    fio_x509_keypair_clear(&kp);
    return -1;
  }

  *der = buf;
  *der_len = written;
  if (kp_out)
    *kp_out = kp;
  else
    fio_x509_keypair_clear(&kp);
  return 0;
}

/* *****************************************************************************
Key schedule sanity (not crypto-vector duplication)
***************************************************************************** */
FIO_SFUNC void test_tls13_key_schedule(void) {
  uint8_t psk[32] = {0};
  uint8_t early1[48], early2[48];
  fio_tls13_derive_early_secret(early1, psk, 32, 0);
  fio_tls13_derive_early_secret(early2, psk, 32, 0);
  FIO_ASSERT(!FIO_MEMCMP(early1, early2, 32),
             "early secret derivation not deterministic");

  uint8_t ecdhe1[32], ecdhe2[32];
  fio_rand_bytes(ecdhe1, 32);
  fio_rand_bytes(ecdhe2, 32);
  uint8_t hs1[48], hs2[48];
  fio_tls13_derive_handshake_secret(hs1, early1, ecdhe1, 32, 0);
  fio_tls13_derive_handshake_secret(hs2, early1, ecdhe2, 32, 0);
  FIO_ASSERT(FIO_MEMCMP(hs1, hs2, 32) != 0,
             "different ECDHE secrets produced identical handshake secret");

  uint8_t master[48];
  fio_tls13_derive_master_secret(master, hs1, 0);
  FIO_ASSERT(FIO_MEMCMP(master, hs1, 32) != 0,
             "master secret equals handshake secret");

  uint8_t traffic_secret[48];
  uint8_t transcript[32] = {0};
  fio_tls13_derive_secret(traffic_secret,
                          master,
                          32,
                          "c ap traffic",
                          12,
                          transcript,
                          32,
                          0);

  uint8_t key[32], iv[12];
  fio_tls13_derive_traffic_keys(key, 16, iv, traffic_secret, 0);
  uint8_t key2[32], iv2[12];
  fio_tls13_derive_traffic_keys(key2, 16, iv2, traffic_secret, 0);
  FIO_ASSERT(!FIO_MEMCMP(key, key2, 16) && !FIO_MEMCMP(iv, iv2, 12),
             "traffic key derivation not deterministic");

  uint8_t finished_key[48];
  fio_tls13_derive_finished_key(finished_key, traffic_secret, 0);
  FIO_ASSERT(FIO_MEMCMP(finished_key, key, 16) != 0,
             "finished key equals traffic key");

  uint8_t verify[32];
  fio_tls13_compute_finished(verify, finished_key, transcript, 0);
  uint8_t verify2[32];
  fio_tls13_compute_finished(verify2, finished_key, transcript, 0);
  FIO_ASSERT(!FIO_MEMCMP(verify, verify2, 32),
             "finished verify_data not deterministic");

  uint8_t updated[48];
  fio_tls13_update_traffic_secret(updated, traffic_secret, 0);
  FIO_ASSERT(FIO_MEMCMP(updated, traffic_secret, 32) != 0,
             "updated traffic secret equals current secret");

  /* SHA-384 variant produces different length material. */
  uint8_t early384[48];
  fio_tls13_derive_early_secret(early384, NULL, 0, 1);
  FIO_ASSERT(FIO_MEMCMP(early1, early384, 32) != 0,
             "SHA-256 and SHA-384 early secrets match");
}

/* *****************************************************************************
Handshake message parsing/building
***************************************************************************** */
FIO_SFUNC void test_tls13_handshake_messages(void) {
  uint8_t random[32];
  fio_rand_bytes(random, 32);
  uint8_t x25519_pk[32];
  fio_rand_bytes(x25519_pk, 32);

  uint8_t ch[2048];
  int ch_len = fio_tls13_build_client_hello(ch,
                                              sizeof(ch),
                                              random,
                                              "localhost",
                                              x25519_pk,
                                              NULL,
                                              0);
  FIO_ASSERT(ch_len > 0, "ClientHello build failed");

  fio_tls13_handshake_type_e msg_type;
  size_t body_len;
  const uint8_t *body =
      fio_tls13_parse_handshake_header(ch, (size_t)ch_len, &msg_type, &body_len);
  FIO_ASSERT(body != NULL, "ClientHello header parse failed");
  FIO_ASSERT(msg_type == FIO_TLS13_HS_CLIENT_HELLO,
             "ClientHello message type mismatch");
  FIO_ASSERT(body_len + 4 == (size_t)ch_len, "ClientHello body length mismatch");

  /* ServerHello with HRR magic random */
  uint8_t hrr[128];
  uint8_t *p = hrr + 4;
  uint8_t *start = p;
  p[0] = 0x03;
  p[1] = 0x03;
  p += 2;
  FIO_MEMCPY(p, FIO_TLS13_HRR_RANDOM, 32);
  p += 32;
  *p++ = 0; /* session id len */
  p[0] = 0x13;
  p[1] = 0x01;
  p += 2;
  *p++ = 0; /* compression */
  /* extensions: supported_versions + key_share (group only) */
  uint8_t *ext_len_ptr = p;
  p += 2;
  uint8_t *ext_start = p;
  p[0] = 0x00;
  p[1] = 43;
  p[2] = 0x00;
  p[3] = 0x02;
  p[4] = 0x03;
  p[5] = 0x04;
  p += 6;
  p[0] = 0x00;
  p[1] = 51;
  p[2] = 0x00;
  p[3] = 0x02;
  p[4] = 0x00;
  p[5] = 23; /* secp256r1 */
  p += 6;
  ext_len_ptr[0] = (uint8_t)((p - ext_start) >> 8);
  ext_len_ptr[1] = (uint8_t)(p - ext_start);
  size_t hrr_body = (size_t)(p - start);
  fio_tls13_write_handshake_header(hrr, FIO_TLS13_HS_SERVER_HELLO, hrr_body);

  fio_tls13_server_hello_s sh;
  FIO_ASSERT(fio_tls13_parse_server_hello(&sh, hrr + 4, hrr_body) == 0,
             "HelloRetryRequest parse failed");
  FIO_ASSERT(sh.is_hello_retry_request == 1,
             "HRR magic random not detected");
  FIO_ASSERT(sh.key_share_group == FIO_TLS13_GROUP_SECP256R1,
             "HRR selected group mismatch");

  /* EncryptedExtensions with ALPN response. */
  uint8_t ee[64];
  uint8_t *ep = ee + 4;
  uint8_t *ee_start = ep;
  ep += 2; /* ext_len */
  uint8_t *ee_ext_start = ep;
  *ep++ = 0x00;
  *ep++ = 16; /* ALPN */
  *ep++ = 0x00;
  *ep++ = 5;  /* ext data len */
  *ep++ = 0x00;
  *ep++ = 3;  /* list len */
  *ep++ = 2;
  ep[0] = 'h';
  ep[1] = '2';
  ep += 2;
  ee_start[0] = (uint8_t)((ep - ee_ext_start) >> 8);
  ee_start[1] = (uint8_t)(ep - ee_ext_start);
  size_t ee_body = (size_t)(ep - ee_start);
  fio_tls13_write_handshake_header(ee, FIO_TLS13_HS_ENCRYPTED_EXTENSIONS, ee_body);

  fio_tls13_encrypted_extensions_s ee_out;
  FIO_ASSERT(fio_tls13_parse_encrypted_extensions(&ee_out, ee + 4, ee_body) ==
                 0,
             "EncryptedExtensions parse failed");
  FIO_ASSERT(ee_out.alpn_selected_len == 2,
             "ALPN selected length mismatch");
  FIO_ASSERT(!FIO_MEMCMP(ee_out.alpn_selected, "h2", 2),
             "ALPN selected value mismatch");

  /* CertificateRequest build/parse. */
  uint16_t sig_algs[] = {FIO_TLS13_SIG_ECDSA_SECP256R1_SHA256};
  uint8_t context[8];
  fio_rand_bytes(context, sizeof(context));
  uint8_t cr[64];
  int cr_len = fio_tls13_build_certificate_request(cr,
                                                    sizeof(cr),
                                                    context,
                                                    sizeof(context),
                                                    sig_algs,
                                                    1);
  FIO_ASSERT(cr_len > 0, "CertificateRequest build failed");

  fio_tls13_certificate_request_s cr_out;
  FIO_ASSERT(fio_tls13_parse_certificate_request(&cr_out, cr + 4,
                                                  cr_len - 4) == 0,
             "CertificateRequest parse failed");
  FIO_ASSERT(cr_out.signature_algorithm_count == 1,
             "CertificateRequest sig alg count mismatch");
  FIO_ASSERT(cr_out.signature_algorithms[0] ==
                 FIO_TLS13_SIG_ECDSA_SECP256R1_SHA256,
             "CertificateRequest sig alg mismatch");
  FIO_ASSERT(cr_out.certificate_request_context_len == sizeof(context),
             "CertificateRequest context length mismatch");

  /* Finished build/parse. */
  uint8_t verify_data[32];
  fio_rand_bytes(verify_data, 32);
  uint8_t fin[64];
  int fin_len = fio_tls13_build_finished(fin, sizeof(fin), verify_data, 32);
  FIO_ASSERT(fin_len == 36, "Finished build length mismatch");
  FIO_ASSERT(fio_tls13_parse_finished(fin + 4, 32, verify_data, 32) == 0,
             "Finished verification failed");

  /* KeyUpdate build/parse. */
  uint8_t ku[8];
  int ku_len = fio_tls13_build_key_update(
      ku, sizeof(ku), FIO_TLS13_KEY_UPDATE_REQUESTED);
  FIO_ASSERT(ku_len == 5, "KeyUpdate build length mismatch");
  int req = -1;
  FIO_ASSERT(fio_tls13_parse_key_update(ku + 4, 1, &req) == 0,
             "KeyUpdate parse failed");
  FIO_ASSERT(req == FIO_TLS13_KEY_UPDATE_REQUESTED,
             "KeyUpdate request value mismatch");
}

/* *****************************************************************************
KeyUpdate key rotation
***************************************************************************** */
FIO_SFUNC void test_tls13_key_update(void) {
  uint8_t secret_rx[48], secret_tx[48];
  fio_rand_bytes(secret_rx, 32);
  FIO_MEMCPY(secret_tx, secret_rx, 32);

  uint8_t key[32], iv[12];
  fio_tls13_derive_traffic_keys(key, 16, iv, secret_rx, 0);

  fio_tls13_record_keys_s rx_keys, tx_keys;
  fio_tls13_record_keys_init(
      &rx_keys, key, 16, iv, FIO_TLS13_CIPHER_AES_128_GCM);
  fio_tls13_record_keys_init(
      &tx_keys, key, 16, iv, FIO_TLS13_CIPHER_AES_128_GCM);

  fio_tls13_record_keys_s old_rx = rx_keys;
  fio_tls13_record_keys_s old_tx = tx_keys;

  uint8_t ku[8];
  int ku_len = fio_tls13_build_key_update(
      ku, sizeof(ku), FIO_TLS13_KEY_UPDATE_REQUESTED);
  FIO_ASSERT(ku_len == 5, "KeyUpdate build failed");

  uint8_t pending = 0;
  FIO_ASSERT(fio_tls13_process_key_update(secret_rx,
                                          &rx_keys,
                                          ku + 4,
                                          1,
                                          &pending,
                                          0,
                                          16,
                                          FIO_TLS13_CIPHER_AES_128_GCM) == 0,
             "process_key_update failed");
  FIO_ASSERT(pending == 1, "key_update_pending not set");
  FIO_ASSERT(FIO_MEMCMP(rx_keys.key, old_rx.key, 16) != 0,
             "receiving key did not change");
  FIO_ASSERT(rx_keys.sequence_number == 0,
             "receiving sequence number not reset");

  uint8_t response[64];
  int resp_len = fio_tls13_send_key_update_response(response,
                                                    sizeof(response),
                                                    secret_tx,
                                                    &tx_keys,
                                                    &pending,
                                                    0,
                                                    16,
                                                    FIO_TLS13_CIPHER_AES_128_GCM);
  FIO_ASSERT(resp_len > 0, "send_key_update_response failed");
  FIO_ASSERT(pending == 0, "key_update_pending not cleared");
  FIO_ASSERT(FIO_MEMCMP(tx_keys.key, old_tx.key, 16) != 0,
             "sending key did not change");
  FIO_ASSERT(tx_keys.sequence_number == 0,
             "sending sequence number not reset");

  /* Response decrypts with old receiving keys (peer hasn't updated yet). */
  uint8_t dec[16];
  fio_tls13_content_type_e ct;
  FIO_ASSERT(fio_tls13_record_decrypt(dec,
                                      sizeof(dec),
                                      &ct,
                                      response,
                                      (size_t)resp_len,
                                      &old_rx) == 5,
             "KeyUpdate response decrypt failed");
  FIO_ASSERT(ct == FIO_TLS13_CONTENT_HANDSHAKE,
             "KeyUpdate response content type mismatch");

  fio_tls13_record_keys_clear(&rx_keys);
  fio_tls13_record_keys_clear(&tx_keys);
  fio_tls13_record_keys_clear(&old_rx);
  fio_tls13_record_keys_clear(&old_tx);
}

/* *****************************************************************************
In-memory handshake pump
***************************************************************************** */
FIO_SFUNC void tls13_test_init_server(fio_tls13_server_s *server,
                                      uint8_t **cert_der,
                                      size_t *cert_len,
                                      fio_x509_keypair_s *kp,
                                      const uint8_t **cert_ptrs,
                                      size_t *cert_lens) {
  FIO_ASSERT(tls13_test_make_p256_cert(cert_der, cert_len, kp) == 0,
             "server cert generation failed");
  fio_tls13_server_init(server);
  cert_ptrs[0] = *cert_der;
  cert_lens[0] = *cert_len;
  fio_tls13_server_set_cert_chain(server, cert_ptrs, cert_lens, 1);
  fio_tls13_server_set_private_key(
      server, kp->secret_key, kp->secret_key_len,
      FIO_TLS13_SIG_ECDSA_SECP256R1_SHA256);
  FIO_MEMCPY(server->public_key, kp->public_key, 65);
}

FIO_SFUNC int tls13_test_run_handshake(fio_tls13_client_s *client,
                                       fio_tls13_server_s *server,
                                       int with_client_cert,
                                       fio_x509_keypair_s *client_kp,
                                       uint8_t *client_cert_der,
                                       size_t client_cert_len) {
  if (with_client_cert) {
    fio_tls13_server_require_client_cert(server, 2);
    fio_tls13_client_set_cert(client,
                              client_cert_der,
                              client_cert_len,
                              client_kp->secret_key,
                              client_kp->secret_key_len,
                              FIO_TLS13_SIG_ECDSA_SECP256R1_SHA256);
    fio_tls13_client_set_public_key(client, client_kp->public_key);
  }

  uint8_t client_out[32768];
  uint8_t server_out[32768];
  size_t client_out_len = 0;
  size_t server_out_len = 0;

  int ch_len = fio_tls13_client_start(client, client_out, sizeof(client_out));
  FIO_ASSERT(ch_len > 0, "client start failed");
  client_out_len = (size_t)ch_len;

  for (int round = 0; round < 32; ++round) {
    if (!fio_tls13_server_is_connected(server) && client_out_len > 0) {
      size_t consumed = 0;
      server_out_len = 0;
      while (consumed < client_out_len) {
        size_t out_len = 0;
        int n = fio_tls13_server_process(server,
                                         client_out + consumed,
                                         client_out_len - consumed,
                                         server_out,
                                         sizeof(server_out),
                                         &out_len);
        if (n <= 0) {
          fprintf(stderr,
                  "server process failed round=%d state=%s alert=%d\n",
                  round,
                  fio_tls13_server_state_name(server),
                  server->alert_description);
          return 0;
        }
        consumed += (size_t)n;
        if (out_len > 0 && server_out_len == 0)
          server_out_len = out_len;
      }
      client_out_len = 0;
    }

    if (!fio_tls13_client_is_connected(client) && server_out_len > 0) {
      size_t consumed = 0;
      client_out_len = 0;
      while (consumed < server_out_len) {
        size_t out_len = 0;
        int n = fio_tls13_client_process(client,
                                         server_out + consumed,
                                         server_out_len - consumed,
                                         client_out,
                                         sizeof(client_out),
                                         &out_len);
        if (n <= 0) {
          fprintf(stderr,
                  "client process failed round=%d state=%s alert=%d\n",
                  round,
                  fio_tls13_client_state_name(client),
                  client->alert_description);
          return 0;
        }
        consumed += (size_t)n;
        if (out_len > 0 && client_out_len == 0)
          client_out_len = out_len;
      }
      server_out_len = 0;
    }

    if (fio_tls13_client_is_connected(client) &&
        fio_tls13_server_is_connected(server)) {
      return 1;
    }
  }

  return 0;
}

FIO_SFUNC void test_tls13_handshake_roundtrip(void) {
  uint8_t *server_cert = NULL;
  size_t server_cert_len = 0;
  fio_x509_keypair_s server_kp;
  fio_tls13_server_s server;
  const uint8_t *server_certs[1];
  size_t server_cert_lens[1];
  tls13_test_init_server(&server, &server_cert, &server_cert_len, &server_kp, server_certs, server_cert_lens);

  fio_tls13_client_s client;
  fio_tls13_client_init(&client, "localhost");
  fio_tls13_client_skip_verification(&client, 1);

  FIO_ASSERT(tls13_test_run_handshake(&client, &server, 0, NULL, NULL, 0),
             "handshake roundtrip failed");

  fio_tls13_client_destroy(&client);
  fio_tls13_server_destroy(&server);
  tls13_test_free(server_cert, server_cert_len);
  fio_x509_keypair_clear(&server_kp);
}

FIO_SFUNC void test_tls13_handshake_alpn(void) {
  uint8_t *server_cert = NULL;
  size_t server_cert_len = 0;
  fio_x509_keypair_s server_kp;
  fio_tls13_server_s server;
  const uint8_t *server_certs[1];
  size_t server_cert_lens[1];
  tls13_test_init_server(&server, &server_cert, &server_cert_len, &server_kp, server_certs, server_cert_lens);
  fio_tls13_server_alpn_set(&server, "h2,http/1.1");

  fio_tls13_client_s client;
  fio_tls13_client_init(&client, "localhost");
  fio_tls13_client_skip_verification(&client, 1);
  fio_tls13_client_alpn_set(&client, "h2,http/1.1");

  FIO_ASSERT(tls13_test_run_handshake(&client, &server, 0, NULL, NULL, 0),
             "ALPN handshake failed");
  FIO_ASSERT(client.alpn_selected_len == 2,
             "client ALPN not selected");
  FIO_ASSERT(!FIO_MEMCMP(client.alpn_selected, "h2", 2),
             "client ALPN value wrong");
  FIO_ASSERT(server.selected_alpn_len == 2,
             "server ALPN not selected");
  FIO_ASSERT(!FIO_MEMCMP(server.selected_alpn, "h2", 2),
             "server ALPN value wrong");

  fio_tls13_client_destroy(&client);
  fio_tls13_server_destroy(&server);
  tls13_test_free(server_cert, server_cert_len);
  fio_x509_keypair_clear(&server_kp);
}

FIO_SFUNC void test_tls13_handshake_client_cert(void) {
  uint8_t *server_cert = NULL;
  size_t server_cert_len = 0;
  fio_x509_keypair_s server_kp;
  fio_tls13_server_s server;
  const uint8_t *server_certs[1];
  size_t server_cert_lens[1];
  tls13_test_init_server(&server, &server_cert, &server_cert_len, &server_kp, server_certs, server_cert_lens);

  fio_tls13_client_s client;
  fio_tls13_client_init(&client, "localhost");
  fio_tls13_client_skip_verification(&client, 1);

  uint8_t *client_cert = NULL;
  size_t client_cert_len = 0;
  fio_x509_keypair_s client_kp;
  FIO_ASSERT(tls13_test_make_p256_cert(&client_cert, &client_cert_len,
                                        &client_kp) == 0,
             "client cert generation failed");

  FIO_ASSERT(tls13_test_run_handshake(
                 &client, &server, 1, &client_kp, client_cert, client_cert_len),
             "client-cert handshake failed");
  FIO_ASSERT(fio_tls13_server_client_cert_received(&server),
             "server did not receive client certificate");
  FIO_ASSERT(fio_tls13_server_client_cert_verified(&server),
             "server did not verify client certificate");

  fio_tls13_client_destroy(&client);
  fio_tls13_server_destroy(&server);
  tls13_test_free(server_cert, server_cert_len);
  tls13_test_free(client_cert, client_cert_len);
  fio_x509_keypair_clear(&server_kp);
  fio_x509_keypair_clear(&client_kp);
}

/* *****************************************************************************
Application data roundtrip and large transfer
***************************************************************************** */
FIO_SFUNC void test_tls13_app_data(void) {
  uint8_t *server_cert = NULL;
  size_t server_cert_len = 0;
  fio_x509_keypair_s server_kp;
  fio_tls13_server_s server;
  const uint8_t *server_certs[1];
  size_t server_cert_lens[1];
  tls13_test_init_server(&server, &server_cert, &server_cert_len, &server_kp, server_certs, server_cert_lens);

  fio_tls13_client_s client;
  fio_tls13_client_init(&client, "localhost");
  fio_tls13_client_skip_verification(&client, 1);

  FIO_ASSERT(tls13_test_run_handshake(&client, &server, 0, NULL, NULL, 0),
             "handshake for app data failed");

  uint8_t plaintext[1024];
  fio_rand_bytes(plaintext, sizeof(plaintext));

  uint8_t ct[2048];
  int ct_len = fio_tls13_client_encrypt(
      &client, ct, sizeof(ct), plaintext, sizeof(plaintext));
  FIO_ASSERT(ct_len > 0, "client encrypt failed");

  uint8_t decrypted[2048];
  int dec_len = fio_tls13_server_decrypt(
      &server, decrypted, sizeof(decrypted), ct, (size_t)ct_len);
  FIO_ASSERT(dec_len == (int)sizeof(plaintext), "server decrypt length wrong");
  FIO_ASSERT(!FIO_MEMCMP(decrypted, plaintext, sizeof(plaintext)),
             "server decrypt data mismatch");

  uint8_t reply[512];
  fio_rand_bytes(reply, sizeof(reply));
  ct_len = fio_tls13_server_encrypt(
      &server, ct, sizeof(ct), reply, sizeof(reply));
  FIO_ASSERT(ct_len > 0, "server encrypt failed");

  dec_len = fio_tls13_client_decrypt(
      &client, decrypted, sizeof(decrypted), ct, (size_t)ct_len);
  FIO_ASSERT(dec_len == (int)sizeof(reply), "client decrypt length wrong");
  FIO_ASSERT(!FIO_MEMCMP(decrypted, reply, sizeof(reply)),
             "client decrypt data mismatch");

  fio_tls13_client_destroy(&client);
  fio_tls13_server_destroy(&server);
  tls13_test_free(server_cert, server_cert_len);
  fio_x509_keypair_clear(&server_kp);
}

FIO_SFUNC void test_tls13_large_transfer(void) {
  uint8_t *server_cert = NULL;
  size_t server_cert_len = 0;
  fio_x509_keypair_s server_kp;
  fio_tls13_server_s server;
  const uint8_t *server_certs[1];
  size_t server_cert_lens[1];
  tls13_test_init_server(&server, &server_cert, &server_cert_len, &server_kp, server_certs, server_cert_lens);

  fio_tls13_client_s client;
  fio_tls13_client_init(&client, "localhost");
  fio_tls13_client_skip_verification(&client, 1);

  FIO_ASSERT(tls13_test_run_handshake(&client, &server, 0, NULL, NULL, 0),
             "handshake for large transfer failed");

  size_t total = 1024 * 1024;
  size_t chunk = FIO_TLS13_MAX_PLAINTEXT_LEN;
  uint8_t *source = (uint8_t *)tls13_test_alloc(total);
  uint8_t *received = (uint8_t *)tls13_test_alloc(total);
  FIO_ASSERT(source && received, "allocation failed");
  fio_rand_bytes(source, total);

  uint8_t *ct = (uint8_t *)tls13_test_alloc(
      FIO_TLS13_RECORD_HEADER_LEN + chunk + 1 + FIO_TLS13_TAG_LEN);
  FIO_ASSERT(ct, "allocation failed");

  size_t sent = 0;
  size_t recv_pos = 0;
  while (sent < total) {
    size_t n = total - sent;
    if (n > chunk)
      n = chunk;
    int ct_len = fio_tls13_client_encrypt(
        &client, ct,
        FIO_TLS13_RECORD_HEADER_LEN + chunk + 1 + FIO_TLS13_TAG_LEN,
        source + sent, n);
    FIO_ASSERT(ct_len > 0, "large encrypt failed");

    int dec_len = fio_tls13_server_decrypt(
        &server, received + recv_pos, total - recv_pos, ct, (size_t)ct_len);
    FIO_ASSERT(dec_len == (int)n, "large decrypt length wrong");

    sent += n;
    recv_pos += (size_t)dec_len;
  }

  FIO_ASSERT(!FIO_MEMCMP(source, received, total),
             "large transfer data mismatch");

  tls13_test_free(source, total);
  tls13_test_free(received, total);
  tls13_test_free(
      ct, FIO_TLS13_RECORD_HEADER_LEN + chunk + 1 + FIO_TLS13_TAG_LEN);
  fio_tls13_client_destroy(&client);
  fio_tls13_server_destroy(&server);
  tls13_test_free(server_cert, server_cert_len);
  fio_x509_keypair_clear(&server_kp);
}

/* *****************************************************************************
TLS I/O function registration
***************************************************************************** */
FIO_SFUNC void test_tls13_io_functions(void) {
  fio_io_functions_s fn = fio_tls13_io_functions();
  FIO_ASSERT(fn.build_context != NULL, "build_context missing");
  FIO_ASSERT(fn.free_context != NULL, "free_context missing");
  FIO_ASSERT(fn.start != NULL, "start missing");
  FIO_ASSERT(fn.read != NULL, "read missing");
  FIO_ASSERT(fn.write != NULL, "write missing");
  FIO_ASSERT(fn.flush != NULL, "flush missing");
  FIO_ASSERT(fn.cleanup != NULL, "cleanup missing");
  FIO_ASSERT(fn.finish != NULL, "finish missing");
}

/* *****************************************************************************
Main
***************************************************************************** */
int main(void) {
  test_tls13_key_schedule();
  test_tls13_handshake_messages();
  test_tls13_key_update();
  test_tls13_handshake_roundtrip();
  test_tls13_handshake_alpn();
  test_tls13_handshake_client_cert();
  test_tls13_app_data();
  test_tls13_large_transfer();
  test_tls13_io_functions();
  return 0;
}
