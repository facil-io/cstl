/* *****************************************************************************
Test for fio-stl/190 tls13.h and fio-stl/405 tls13.h — TLS 1.3 correctness

Coverage: key schedule derivation, handshake message parsing/building,
client/server in-memory handshake roundtrip, ALPN negotiation, client
certificate authentication, KeyUpdate key rotation, large application-data
transfer, TLS I/O function registration, and compressed EC points (SEC 1) in
certificates / key files, incl. OpenSSL interoperability (memory BIOs).  Crypto
algorithm vectors are intentionally omitted (those live in sha.c, aes.c,
chacha.c, rsa.c, x509.c, ed25519.c, p256.c, p384.c, hkdf.c).
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

  fio_buf_info_s san_buf[] = {FIO_BUF_INFO2((char *)"localhost", 9)};
  fio_x509_cert_options_s opts = {
      .cn = FIO_BUF_INFO2((char *)"localhost", 9),
      .org = FIO_BUF_INFO2((char *)"facil.io", 8),
      .san_dns = san_buf,
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
Hostile limits: ClientHello builders never write past their capacity
***************************************************************************** */
FIO_SFUNC void test_tls13_client_hello_limits(void) {
  uint8_t random[32] = {7};
  uint8_t x25519_pk[32] = {9};
  char sni[256];
  FIO_MEMSET(sni, 'a', 255);
  sni[255] = 0;
  uint8_t buf[4096];

  /* 255-byte SNI into a 256-byte buffer: must fail without overflow. */
  FIO_MEMSET(buf, 0xCC, sizeof(buf));
  FIO_ASSERT(
      fio_tls13_build_client_hello(buf, 256, random, sni, x25519_pk, NULL, 0) ==
          -1,
      "oversized ClientHello must fail");
  for (size_t i = 256; i < sizeof(buf); ++i)
    FIO_ASSERT(buf[i] == 0xCC, "ClientHello wrote past capacity");

  /* Exact capacity succeeds; one byte less fails. */
  int len = fio_tls13_build_client_hello(buf,
                                         sizeof(buf),
                                         random,
                                         sni,
                                         x25519_pk,
                                         NULL,
                                         0);
  FIO_ASSERT(len > 256, "ClientHello with 255-byte SNI failed");
  FIO_ASSERT(fio_tls13_build_client_hello(buf,
                                          (size_t)len,
                                          random,
                                          sni,
                                          x25519_pk,
                                          NULL,
                                          0) == len,
             "exact capacity ClientHello failed");
  FIO_ASSERT(fio_tls13_build_client_hello(buf,
                                          (size_t)len - 1,
                                          random,
                                          sni,
                                          x25519_pk,
                                          NULL,
                                          0) == -1,
             "short capacity ClientHello must fail");

  /* Large cipher suite counts must not overflow the buffer. */
  uint16_t suites[512];
  for (size_t i = 0; i < 512; ++i)
    suites[i] = FIO_TLS13_CIPHER_SUITE_AES_128_GCM_SHA256;
  FIO_MEMSET(buf, 0xCC, sizeof(buf));
  FIO_ASSERT(fio_tls13_build_client_hello(buf,
                                          512,
                                          random,
                                          "localhost",
                                          x25519_pk,
                                          suites,
                                          512) == -1,
             "large suite count must fail");
  for (size_t i = 512; i < sizeof(buf); ++i)
    FIO_ASSERT(buf[i] == 0xCC, "ClientHello suites wrote past capacity");

  /* HelloRetryRequest cookie larger than the output: CH2 must fail safely. */
  fio_tls13_client_s client;
  fio_tls13_client_init(&client, sni);
  client.hrr.received = 1; /* cookie-only HRR: CH1 shares are resent */
  client.hrr.cookie.len = 2000;
  client.hrr.cookie.buf =
      (uint8_t *)FIO_MEM_REALLOC(NULL, 0, client.hrr.cookie.len, 0);
  FIO_ASSERT_ALLOC(client.hrr.cookie.buf);
  FIO_MEMSET(client.hrr.cookie.buf, 0x5A, client.hrr.cookie.len);
  FIO_MEMSET(buf, 0xCC, sizeof(buf));
  FIO_ASSERT(fio___tls13_build_client_hello_record(&client, buf, 2048) == -1,
             "CH2 with an oversized cookie must fail");
  for (size_t i = 2048; i < sizeof(buf); ++i)
    FIO_ASSERT(buf[i] == 0xCC, "CH2 wrote past capacity");
  len = fio___tls13_build_client_hello_record(&client, buf, sizeof(buf));
  FIO_ASSERT(len > 2000 && (size_t)len <= sizeof(buf),
             "CH2 with a large cookie and enough capacity failed");
  fio_tls13_client_destroy(&client);
}

/* *****************************************************************************
HelloRetryRequest / ServerHello group and cipher suite rules (RFC 8446 4.1.4)
***************************************************************************** */

/* Builds a plaintext ServerHello (or HRR) record. `group`==0 omits key_share;
 * `share_len`==0 writes an HRR-style key_share (group only). */
FIO_SFUNC size_t tls13_test_fake_sh(uint8_t *out,
                                    int hrr,
                                    uint16_t suite,
                                    uint16_t group,
                                    size_t share_len,
                                    const uint8_t *share,
                                    size_t cookie_len) {
  uint8_t *p = out + 9; /* record header (5) + handshake header (4) */
  fio___tls13_write_u16(p, FIO_TLS13_VERSION_TLS12);
  p += 2;
  if (hrr)
    FIO_MEMCPY(p, fio___tls13_hrr_random, 32);
  else
    FIO_MEMSET(p, 0x42, 32);
  p += 32;
  *p++ = 0; /* legacy_session_id */
  fio___tls13_write_u16(p, suite);
  p += 2;
  *p++ = 0; /* compression */
  uint8_t *ext_len = p;
  p += 2;
  fio___tls13_write_u16(p, FIO_TLS13_EXT_SUPPORTED_VERSIONS);
  fio___tls13_write_u16(p + 2, 2);
  fio___tls13_write_u16(p + 4, FIO_TLS13_VERSION_TLS13);
  p += 6;
  if (group) {
    fio___tls13_write_u16(p, FIO_TLS13_EXT_KEY_SHARE);
    fio___tls13_write_u16(p + 2, (uint16_t)(share_len ? 4 + share_len : 2));
    fio___tls13_write_u16(p + 4, group);
    p += 6;
    if (share_len) {
      fio___tls13_write_u16(p, (uint16_t)share_len);
      if (share)
        FIO_MEMCPY(p + 2, share, share_len);
      else
        FIO_MEMSET(p + 2, 0x04, share_len);
      p += 2 + share_len;
    }
  }
  if (cookie_len) {
    fio___tls13_write_u16(p, FIO_TLS13_EXT_COOKIE);
    fio___tls13_write_u16(p + 2, (uint16_t)(2 + cookie_len));
    fio___tls13_write_u16(p + 4, (uint16_t)cookie_len);
    FIO_MEMSET(p + 6, 0xC0, cookie_len);
    p += 6 + cookie_len;
  }
  fio___tls13_write_u16(ext_len, (uint16_t)(p - ext_len - 2));
  size_t body_len = (size_t)(p - out) - 9;
  fio___tls13_write_record_header(out,
                                  FIO_TLS13_CONTENT_HANDSHAKE,
                                  (uint16_t)(body_len + 4));
  fio_tls13_write_handshake_header(out + 5,
                                   FIO_TLS13_HS_SERVER_HELLO,
                                   body_len);
  return (size_t)(p - out);
}

/* Starts a client, feeds `rec`; returns client_process() and stores CH1/CH2 */
FIO_SFUNC int tls13_test_feed_sh(fio_tls13_client_s *c,
                                 int use_hybrid,
                                 const uint8_t *rec,
                                 size_t rec_len,
                                 uint8_t *ch1,
                                 int *ch1_len,
                                 uint8_t *ch2,
                                 size_t *ch2_len) {
  fio_tls13_client_init(c, "localhost");
  fio_tls13_client_alpn_set(c, "h2,http/1.1");
  if (!use_hybrid)
    c->use_hybrid = 0;
  *ch1_len = fio_tls13_client_start(c, ch1, 4096);
  FIO_ASSERT(*ch1_len > 0, "client start failed");
  *ch2_len = 0;
  return fio_tls13_client_process(c, rec, rec_len, ch2, 8192, ch2_len);
}

FIO_SFUNC void test_tls13_hello_retry_rules(void) {
  static uint8_t rec[4096], ch1[4096], ch2[8192];
  fio_tls13_client_s c;
  int ch1_len;
  size_t ch2_len, rec_len;
  const uint16_t aes = FIO_TLS13_CIPHER_SUITE_AES_128_GCM_SHA256;
  /* CH layout: record(5) hs(4) version(2) random(32) sid(1) suites(2+6)
   * compression(2), then extensions length (2) at offset 54. */
  const size_t ext_at = 54;

  /* Cookie-only HRR: CH2 == CH1 (hybrid shares, ALPN, ...) + cookie */
  rec_len = tls13_test_fake_sh(rec, 1, aes, 0, 0, NULL, 40);
  FIO_ASSERT(
      tls13_test_feed_sh(&c, 1, rec, rec_len, ch1, &ch1_len, ch2, &ch2_len) ==
          (int)rec_len,
      "cookie-only HRR rejected (alert %d)",
      c.alert_description);
  FIO_ASSERT(ch2_len == (size_t)ch1_len + 46, "CH2 length mismatch");
  FIO_ASSERT(!FIO_MEMCMP(ch1 + ext_at + 2,
                         ch2 + ext_at + 2,
                         (size_t)ch1_len - ext_at - 2),
             "CH2 must repeat CH1 extensions (ALPN, key_share, ...)");
  FIO_ASSERT(fio___tls13_read_u16(ch2 + ch1_len) == FIO_TLS13_EXT_COOKIE,
             "CH2 cookie missing");
  {
    static const char alpn[] = "\x02h2\x08http/1.1";
    int has_alpn = 0;
    for (size_t i = ext_at; i + sizeof(alpn) - 1 <= ch2_len && !has_alpn; ++i)
      has_alpn = !FIO_MEMCMP(ch2 + i, alpn, sizeof(alpn) - 1);
    FIO_ASSERT(has_alpn, "CH2 must carry ALPN");
  }
  /* ServerHello2 must keep the HRR cipher suite */
  rec_len = tls13_test_fake_sh(rec,
                               0,
                               FIO_TLS13_CIPHER_SUITE_AES_256_GCM_SHA384,
                               FIO_TLS13_GROUP_X25519,
                               32,
                               NULL,
                               0);
  ch2_len = 0;
  FIO_ASSERT(fio_tls13_client_process(&c, rec, rec_len, ch2, 8192, &ch2_len) ==
                     -1 &&
                 c.alert_description == FIO_TLS13_ALERT_ILLEGAL_PARAMETER,
             "ServerHello2 cipher suite change must be rejected");
  fio_tls13_client_destroy(&c);

  /* HRR without changes (no key_share, no cookie) or selecting a group we
   * already sent a share for: illegal_parameter */
  rec_len = tls13_test_fake_sh(rec, 1, aes, 0, 0, NULL, 0);
  FIO_ASSERT(
      tls13_test_feed_sh(&c, 1, rec, rec_len, ch1, &ch1_len, ch2, &ch2_len) ==
              -1 &&
          c.alert_description == FIO_TLS13_ALERT_ILLEGAL_PARAMETER,
      "no-op HRR must be rejected");
  fio_tls13_client_destroy(&c);
  rec_len = tls13_test_fake_sh(rec, 1, aes, FIO_TLS13_GROUP_X25519, 0, NULL, 0);
  FIO_ASSERT(
      tls13_test_feed_sh(&c, 1, rec, rec_len, ch1, &ch1_len, ch2, &ch2_len) ==
              -1 &&
          c.alert_description == FIO_TLS13_ALERT_ILLEGAL_PARAMETER,
      "HRR selecting X25519 must be rejected");
  fio_tls13_client_destroy(&c);
  rec_len = tls13_test_fake_sh(rec,
                               1,
                               aes,
                               FIO_TLS13_GROUP_X25519MLKEM768,
                               0,
                               NULL,
                               0);
  FIO_ASSERT(
      tls13_test_feed_sh(&c, 0, rec, rec_len, ch1, &ch1_len, ch2, &ch2_len) ==
              -1 &&
          c.alert_description == FIO_TLS13_ALERT_ILLEGAL_PARAMETER,
      "HRR selecting an unoffered hybrid group must be rejected");
  fio_tls13_client_destroy(&c);

#if defined(H___FIO_P256___H)
  /* HRR selecting P-256: CH2 carries one P-256 share; SH2 must use P-256 */
  rec_len =
      tls13_test_fake_sh(rec, 1, aes, FIO_TLS13_GROUP_SECP256R1, 0, NULL, 0);
  FIO_ASSERT(
      tls13_test_feed_sh(&c, 1, rec, rec_len, ch1, &ch1_len, ch2, &ch2_len) ==
              (int)rec_len &&
          ch2_len > 0,
      "HRR selecting P-256 failed");
  rec_len =
      tls13_test_fake_sh(rec, 0, aes, FIO_TLS13_GROUP_X25519, 32, NULL, 0);
  ch2_len = 0;
  FIO_ASSERT(fio_tls13_client_process(&c, rec, rec_len, ch2, 8192, &ch2_len) ==
                     -1 &&
                 c.alert_description == FIO_TLS13_ALERT_ILLEGAL_PARAMETER,
             "ServerHello2 group change must be rejected");
  fio_tls13_client_destroy(&c);
#endif

  /* ServerHello with a share we never sent: illegal_parameter */
#if defined(H___FIO_P256___H)
  {
    uint8_t sk[32], pk[65];
    FIO_ASSERT(!fio_p256_keypair(sk, pk), "P-256 keypair failed");
    rec_len =
        tls13_test_fake_sh(rec, 0, aes, FIO_TLS13_GROUP_SECP256R1, 65, pk, 0);
  }
#else
  rec_len =
      tls13_test_fake_sh(rec, 0, aes, FIO_TLS13_GROUP_SECP256R1, 65, NULL, 0);
#endif
  FIO_ASSERT(
      tls13_test_feed_sh(&c, 1, rec, rec_len, ch1, &ch1_len, ch2, &ch2_len) ==
              -1 &&
          c.alert_description == FIO_TLS13_ALERT_ILLEGAL_PARAMETER,
      "P-256 ServerHello without HRR must be rejected");
  fio_tls13_client_destroy(&c);
  rec_len = tls13_test_fake_sh(rec,
                               0,
                               aes,
                               FIO_TLS13_GROUP_X25519MLKEM768,
                               1120,
                               NULL,
                               0);
  FIO_ASSERT(
      tls13_test_feed_sh(&c, 0, rec, rec_len, ch1, &ch1_len, ch2, &ch2_len) ==
              -1 &&
          c.alert_description == FIO_TLS13_ALERT_ILLEGAL_PARAMETER,
      "hybrid ServerHello without a hybrid share must be rejected");
  fio_tls13_client_destroy(&c);
}

/* *****************************************************************************
Certificate list parsing: malformed entries / suffixes are rejected
***************************************************************************** */
FIO_SFUNC void test_tls13_certificate_list_strict(void) {
  /* body: context(1) + list_len(3) + entries; entry = len(3) + der + ext(2) */
  static uint8_t msg[4 + 4 + 12 * 6 + 2];
  uint8_t *body = msg + 4;
  for (size_t entries = 1; entries <= 12; ++entries) {
    for (int bad = 0; bad < 4; ++bad) {
      size_t list_len = entries * 6;
      uint8_t *p = body + 4;
      body[0] = 0;
      for (size_t i = 0; i < entries; ++i, p += 6)
        FIO_MEMCPY(p, "\x00\x00\x01\x30\x00\x00", 6);
      size_t body_len = 4 + list_len;
      if (bad == 1)
        body[body_len++] = 0; /* trailing data after the list */
      if (bad == 2)
        p[-6 + 2] = 0;        /* zero length certificate */
      if (bad == 3)
        p[-1] = 1;            /* extension overruns the list */
      body[1] = (uint8_t)(list_len >> 16);
      body[2] = (uint8_t)(list_len >> 8);
      body[3] = (uint8_t)list_len;
      fio_tls13_write_handshake_header(msg,
                                       FIO_TLS13_HS_CERTIFICATE,
                                       body_len);

      fio_tls13_client_s c;
      fio_tls13_client_init(&c, "localhost");
      int r = fio___tls13_process_certificate(&c, body, body_len);
      FIO_ASSERT((r == 0) == !bad,
                 "client cert list entries=%zu bad=%d: r=%d",
                 entries,
                 bad,
                 r);
      FIO_ASSERT(bad || c.peer_chain.count == (entries > 10 ? 10 : entries),
                 "client cert count wrong");
      FIO_ASSERT(!bad || c.alert_description == FIO_TLS13_ALERT_DECODE_ERROR,
                 "client cert list: decode_error expected");
      fio_tls13_client_destroy(&c);

      fio_tls13_server_s s;
      fio_tls13_server_init(&s);
      s.state = FIO_TLS13_SERVER_STATE_WAIT_CLIENT_CERT;
      r = fio___tls13_server_process_client_certificate(&s, msg, 4 + body_len);
      FIO_ASSERT((r == 0) == !bad,
                 "server cert list entries=%zu bad=%d: r=%d",
                 entries,
                 bad,
                 r);
      FIO_ASSERT(bad ||
                     s.peer_auth.chain.count == (entries > 10 ? 10 : entries),
                 "server cert count wrong");
      fio_tls13_server_destroy(&s);
    }
  }
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
  const uint8_t *body = fio_tls13_parse_handshake_header(ch,
                                                         (size_t)ch_len,
                                                         &msg_type,
                                                         &body_len);
  FIO_ASSERT(body != NULL, "ClientHello header parse failed");
  FIO_ASSERT(msg_type == FIO_TLS13_HS_CLIENT_HELLO,
             "ClientHello message type mismatch");
  FIO_ASSERT(body_len + 4 == (size_t)ch_len,
             "ClientHello body length mismatch");

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
  FIO_ASSERT(sh.is_hello_retry_request == 1, "HRR magic random not detected");
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
  *ep++ = 5; /* ext data len */
  *ep++ = 0x00;
  *ep++ = 3; /* list len */
  *ep++ = 2;
  ep[0] = 'h';
  ep[1] = '2';
  ep += 2;
  ee_start[0] = (uint8_t)((ep - ee_ext_start) >> 8);
  ee_start[1] = (uint8_t)(ep - ee_ext_start);
  size_t ee_body = (size_t)(ep - ee_start);
  fio_tls13_write_handshake_header(ee,
                                   FIO_TLS13_HS_ENCRYPTED_EXTENSIONS,
                                   ee_body);

  fio_tls13_encrypted_extensions_s ee_out;
  FIO_ASSERT(fio_tls13_parse_encrypted_extensions(&ee_out, ee + 4, ee_body) ==
                 0,
             "EncryptedExtensions parse failed");
  FIO_ASSERT(ee_out.alpn_selected_len == 2, "ALPN selected length mismatch");
  FIO_ASSERT(!FIO_MEMCMP(ee_out.alpn_selected, "h2", 2),
             "ALPN selected value mismatch");

  /* CertificateRequest build/parse. */
  uint16_t signature_algos[] = {FIO_TLS13_SIGNATURE_ECDSA_SECP256R1_SHA256};
  uint8_t context[8];
  fio_rand_bytes(context, sizeof(context));
  uint8_t cr[64];
  int cr_len = fio_tls13_build_certificate_request(
      cr,
      sizeof(cr),
      FIO_UBUF_INFO2(context, sizeof(context)),
      signature_algos,
      1);
  FIO_ASSERT(cr_len > 0, "CertificateRequest build failed");

  fio_tls13_certificate_request_s cr_out;
  FIO_ASSERT(fio_tls13_parse_certificate_request(&cr_out, cr + 4, cr_len - 4) ==
                 0,
             "CertificateRequest parse failed");
  FIO_ASSERT(cr_out.algo_count == 1,
             "CertificateRequest sig alg count mismatch");
  FIO_ASSERT(cr_out.algos[0] == FIO_TLS13_SIGNATURE_ECDSA_SECP256R1_SHA256,
             "CertificateRequest sig alg mismatch");
  FIO_ASSERT(cr_out.context_len == sizeof(context),
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
  int ku_len = fio_tls13_build_key_update(ku,
                                          sizeof(ku),
                                          FIO_TLS13_KEY_UPDATE_REQUESTED);
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
  fio_tls13_record_keys_init(&rx_keys,
                             key,
                             16,
                             iv,
                             FIO_TLS13_CIPHER_AES_128_GCM);
  fio_tls13_record_keys_init(&tx_keys,
                             key,
                             16,
                             iv,
                             FIO_TLS13_CIPHER_AES_128_GCM);

  fio_tls13_record_keys_s old_rx = rx_keys;
  fio_tls13_record_keys_s old_tx = tx_keys;

  uint8_t ku[8];
  int ku_len = fio_tls13_build_key_update(ku,
                                          sizeof(ku),
                                          FIO_TLS13_KEY_UPDATE_REQUESTED);
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
  int resp_len =
      fio_tls13_send_key_update_response(response,
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
  FIO_ASSERT(tx_keys.sequence_number == 0, "sending sequence number not reset");

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
                                      fio_ubuf_info_s *chain) {
  FIO_ASSERT(tls13_test_make_p256_cert(cert_der, cert_len, kp) == 0,
             "server cert generation failed");
  fio_tls13_server_init(server);
  chain[0] = FIO_UBUF_INFO2(*cert_der, *cert_len);
  fio_tls13_server_set_cert_chain(server, chain, 1);
  fio_tls13_server_set_private_key(
      server,
      FIO_UBUF_INFO2(kp->secret_key, kp->secret_key_len),
      FIO_TLS13_SIGNATURE_ECDSA_SECP256R1_SHA256);
  FIO_MEMCPY(server->credentials.public_key, kp->public_key, 65);
}

FIO_SFUNC int tls13_test_run_handshake(fio_tls13_client_s *client,
                                       fio_tls13_server_s *server,
                                       int with_client_cert,
                                       fio_x509_keypair_s *client_kp,
                                       uint8_t *client_cert_der,
                                       size_t client_cert_len) {
  if (with_client_cert) {
    fio_tls13_server_require_client_cert(server, 2);
    fio_tls13_client_set_cert(
        client,
        FIO_UBUF_INFO2(client_cert_der, client_cert_len),
        FIO_UBUF_INFO2(client_kp->secret_key, client_kp->secret_key_len),
        FIO_TLS13_SIGNATURE_ECDSA_SECP256R1_SHA256);
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

/* *****************************************************************************
Handshake fragmentation / coalescing (RFC 8446 Section 5.1)
***************************************************************************** */

/* Re-records a flight: plaintext handshake data then encrypted handshake data
 * (decrypted with a copy of `keys` from sequence 0), split in `frag` bytes. */
FIO_SFUNC size_t tls13_test_refragment(uint8_t *out,
                                       const uint8_t *in,
                                       size_t in_len,
                                       const fio_tls13_record_keys_s *keys,
                                       size_t frag) {
  static uint8_t plain[32768], enc[32768], rec[FIO_TLS13_MAX_CIPHERTEXT_LEN + 5];
  size_t plain_len = 0, enc_len = 0, out_len = 0;
  fio_tls13_record_keys_s k = {0};
  if (keys) {
    k = *keys;
    k.sequence_number = 0;
  }
  while (in_len >= 5) {
    size_t rlen = 5 + fio___tls13_read_u16(in + 3);
    FIO_ASSERT(rlen <= in_len, "refragment: truncated record");
    if (in[0] == FIO_TLS13_CONTENT_HANDSHAKE) {
      FIO_MEMCPY(plain + plain_len, in + 5, rlen - 5);
      plain_len += rlen - 5;
    } else if (in[0] == FIO_TLS13_CONTENT_APPLICATION_DATA) {
      fio_tls13_content_type_e ct;
      FIO_MEMCPY(rec, in, rlen);
      int n = fio_tls13_record_decrypt(enc + enc_len,
                                       sizeof(enc) - enc_len,
                                       &ct,
                                       rec,
                                       rlen,
                                       &k);
      FIO_ASSERT(n >= 0 && ct == FIO_TLS13_CONTENT_HANDSHAKE,
                 "refragment: decrypt failed");
      enc_len += (size_t)n;
    }
    in += rlen;
    in_len -= rlen;
  }
  for (size_t i = 0; i < plain_len; i += frag) {
    size_t n = (plain_len - i < frag) ? plain_len - i : frag;
    fio___tls13_write_record_header(out + out_len,
                                    FIO_TLS13_CONTENT_HANDSHAKE,
                                    (uint16_t)n);
    FIO_MEMCPY(out + out_len + 5, plain + i, n);
    out_len += 5 + n;
  }
  if (keys)
    k.sequence_number = 0;
  for (size_t i = 0; i < enc_len; i += frag) {
    size_t n = (enc_len - i < frag) ? enc_len - i : frag;
    int r = fio_tls13_record_encrypt(out + out_len,
                                     65536,
                                     enc + i,
                                     n,
                                     FIO_TLS13_CONTENT_HANDSHAKE,
                                     &k);
    FIO_ASSERT(r > 0, "refragment: encrypt failed");
    out_len += (size_t)r;
  }
  return out_len;
}

/* Feeds every record of `in`; returns the accumulated output length or -1 */
FIO_SFUNC int tls13_test_feed(void *ctx,
                              int is_server,
                              const uint8_t *in,
                              size_t in_len,
                              uint8_t *out,
                              size_t out_cap) {
  size_t total = 0;
  while (in_len) {
    size_t out_len = 0;
    int n = is_server ? fio_tls13_server_process((fio_tls13_server_s *)ctx,
                                                 in,
                                                 in_len,
                                                 out + total,
                                                 out_cap - total,
                                                 &out_len)
                      : fio_tls13_client_process((fio_tls13_client_s *)ctx,
                                                 in,
                                                 in_len,
                                                 out + total,
                                                 out_cap - total,
                                                 &out_len);
    if (n <= 0)
      return -1;
    total += out_len;
    in += n;
    in_len -= (size_t)n;
  }
  return (int)total;
}

FIO_SFUNC void test_tls13_handshake_fragmentation(void) {
  static uint8_t a[65536], b[65536], c[65536];
  uint8_t *server_cert = NULL;
  size_t server_cert_len = 0;
  fio_x509_keypair_s server_kp;
  fio_tls13_server_s server;
  fio_ubuf_info_s server_chain[1];
  fio_tls13_client_s client;
  tls13_test_init_server(&server,
                         &server_cert,
                         &server_cert_len,
                         &server_kp,
                         server_chain);
  fio_tls13_client_init(&client, "localhost");
  fio_tls13_client_skip_verification(&client, 1);

  /* ClientHello in 50 byte records */
  int len = fio_tls13_client_start(&client, a, sizeof(a));
  FIO_ASSERT(len > 0, "client start failed");
  size_t n = tls13_test_refragment(b, a, (size_t)len, NULL, 50);
  FIO_ASSERT(n > (size_t)len + 5, "ClientHello was not fragmented");
  len = tls13_test_feed(&server, 1, b, n, a, sizeof(a));
  FIO_ASSERT(len > 0, "fragmented ClientHello rejected");

  /* Server flight in 100 byte records (ServerHello plaintext + encrypted) */
  n = tls13_test_refragment(b, a, (size_t)len, &server.server_handshake_keys,
                            100);
  len = tls13_test_feed(&client, 0, b, n, a, sizeof(a));
  FIO_ASSERT(len > 0 && fio_tls13_client_is_connected(&client),
             "fragmented server flight rejected (alert %d)",
             client.alert_description);

  /* Client Finished in 7 byte records (splits the handshake header) */
  n = tls13_test_refragment(b, a, (size_t)len, &client.client_handshake_keys,
                            7);
  FIO_ASSERT(tls13_test_feed(&server, 1, b, n, c, sizeof(c)) >= 0 &&
                 fio_tls13_server_is_connected(&server),
             "fragmented client Finished rejected (alert %d)",
             server.alert_description);
  FIO_ASSERT(!client.hs_buf.buf && !server.hs_buf.buf,
             "reassembly buffers not released after the handshake");
  fio_tls13_client_destroy(&client);
  fio_tls13_server_destroy(&server);

  /* Data after ServerHello in the same record (key change): rejected */
  fio_tls13_server_init(&server);
  fio_tls13_server_set_cert_chain(&server, server_chain, 1);
  fio_tls13_server_set_private_key(
      &server,
      FIO_UBUF_INFO2(server_kp.secret_key, server_kp.secret_key_len),
      FIO_TLS13_SIGNATURE_ECDSA_SECP256R1_SHA256);
  FIO_MEMCPY(server.credentials.public_key, server_kp.public_key, 65);
  fio_tls13_client_init(&client, "localhost");
  fio_tls13_client_skip_verification(&client, 1);
  len = fio_tls13_client_start(&client, a, sizeof(a));
  len = tls13_test_feed(&server, 1, a, (size_t)len, b, sizeof(b));
  FIO_ASSERT(len > 0 && b[0] == FIO_TLS13_CONTENT_HANDSHAKE, "no ServerHello");
  n = 5 + fio___tls13_read_u16(b + 3);
  FIO_MEMCPY(b + n, "\x08\x00\x00\x02\x00\x00", 6); /* EncryptedExtensions */
  fio___tls13_write_u16(b + 3, (uint16_t)(n + 6 - 5));
  FIO_ASSERT(tls13_test_feed(&client, 0, b, n + 6, a, sizeof(a)) == -1 &&
                 client.alert_description ==
                     FIO_TLS13_ALERT_UNEXPECTED_MESSAGE,
             "data after ServerHello in the same record must be rejected");
  fio_tls13_client_destroy(&client);

  /* Declared message length above FIO_TLS13_MAX_HANDSHAKE_MSG: rejected */
  fio_tls13_client_init(&client, "localhost");
  len = fio_tls13_client_start(&client, a, sizeof(a));
  FIO_MEMCPY(b, "\x16\x03\x03\x00\x04\x02\x7F\xFF\xFF", 9);
  FIO_ASSERT(tls13_test_feed(&client, 0, b, 9, a, sizeof(a)) == -1 &&
                 client.alert_description == FIO_TLS13_ALERT_DECODE_ERROR,
             "oversized handshake message must be rejected");
  fio_tls13_client_destroy(&client);

  fio_tls13_server_destroy(&server);
  tls13_test_free(server_cert, server_cert_len);
  fio_x509_keypair_clear(&server_kp);
}

FIO_SFUNC void test_tls13_handshake_roundtrip(void) {
  uint8_t *server_cert = NULL;
  size_t server_cert_len = 0;
  fio_x509_keypair_s server_kp;
  fio_tls13_server_s server;
  fio_ubuf_info_s server_chain[1];
  tls13_test_init_server(&server,
                         &server_cert,
                         &server_cert_len,
                         &server_kp,
                         server_chain);

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
  fio_ubuf_info_s server_chain[1];
  tls13_test_init_server(&server,
                         &server_cert,
                         &server_cert_len,
                         &server_kp,
                         server_chain);
  fio_tls13_server_alpn_set(&server, "h2,http/1.1");

  fio_tls13_client_s client;
  fio_tls13_client_init(&client, "localhost");
  fio_tls13_client_skip_verification(&client, 1);
  fio_tls13_client_alpn_set(&client, "h2,http/1.1");

  FIO_ASSERT(tls13_test_run_handshake(&client, &server, 0, NULL, NULL, 0),
             "ALPN handshake failed");
  FIO_ASSERT(client.alpn.selected_len == 2, "client ALPN not selected");
  FIO_ASSERT(!FIO_MEMCMP(client.alpn.selected, "h2", 2),
             "client ALPN value wrong");
  FIO_ASSERT(server.alpn.selected_len == 2, "server ALPN not selected");
  FIO_ASSERT(!FIO_MEMCMP(server.alpn.selected, "h2", 2),
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
  fio_ubuf_info_s server_chain[1];
  tls13_test_init_server(&server,
                         &server_cert,
                         &server_cert_len,
                         &server_kp,
                         server_chain);

  fio_tls13_client_s client;
  fio_tls13_client_init(&client, "localhost");
  fio_tls13_client_skip_verification(&client, 1);

  uint8_t *client_cert = NULL;
  size_t client_cert_len = 0;
  fio_x509_keypair_s client_kp;
  FIO_ASSERT(tls13_test_make_p256_cert(&client_cert,
                                       &client_cert_len,
                                       &client_kp) == 0,
             "client cert generation failed");

  /* No trust store: mTLS must fail closed (a self-signed certificate and its
   * key must never authenticate on their own). */
  FIO_ASSERT(!tls13_test_run_handshake(&client,
                                       &server,
                                       1,
                                       &client_kp,
                                       client_cert,
                                       client_cert_len),
             "client-cert handshake without a trust store must fail");
  FIO_ASSERT(fio_tls13_server_client_cert_received(&server),
             "server did not receive client certificate");
  FIO_ASSERT(!fio_tls13_server_client_cert_verified(&server),
             "client certificate must not be verified without a trust store");

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
  fio_ubuf_info_s server_chain[1];
  tls13_test_init_server(&server,
                         &server_cert,
                         &server_cert_len,
                         &server_kp,
                         server_chain);

  fio_tls13_client_s client;
  fio_tls13_client_init(&client, "localhost");
  fio_tls13_client_skip_verification(&client, 1);

  FIO_ASSERT(tls13_test_run_handshake(&client, &server, 0, NULL, NULL, 0),
             "handshake for app data failed");

  uint8_t plaintext[1024];
  fio_rand_bytes(plaintext, sizeof(plaintext));

  uint8_t ct[2048];
  int ct_len = fio_tls13_client_encrypt(&client,
                                        ct,
                                        sizeof(ct),
                                        plaintext,
                                        sizeof(plaintext));
  FIO_ASSERT(ct_len > 0, "client encrypt failed");

  uint8_t decrypted[2048];
  int dec_len = fio_tls13_server_decrypt(&server,
                                         decrypted,
                                         sizeof(decrypted),
                                         ct,
                                         (size_t)ct_len);
  FIO_ASSERT(dec_len == (int)sizeof(plaintext), "server decrypt length wrong");
  FIO_ASSERT(!FIO_MEMCMP(decrypted, plaintext, sizeof(plaintext)),
             "server decrypt data mismatch");

  uint8_t reply[512];
  fio_rand_bytes(reply, sizeof(reply));
  ct_len =
      fio_tls13_server_encrypt(&server, ct, sizeof(ct), reply, sizeof(reply));
  FIO_ASSERT(ct_len > 0, "server encrypt failed");

  dec_len = fio_tls13_client_decrypt(&client,
                                     decrypted,
                                     sizeof(decrypted),
                                     ct,
                                     (size_t)ct_len);
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
  fio_ubuf_info_s server_chain[1];
  tls13_test_init_server(&server,
                         &server_cert,
                         &server_cert_len,
                         &server_kp,
                         server_chain);

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

  uint8_t *ct = (uint8_t *)tls13_test_alloc(FIO_TLS13_RECORD_HEADER_LEN +
                                            chunk + 1 + FIO_TLS13_TAG_LEN);
  FIO_ASSERT(ct, "allocation failed");

  size_t sent = 0;
  size_t recv_pos = 0;
  while (sent < total) {
    size_t n = total - sent;
    if (n > chunk)
      n = chunk;
    int ct_len = fio_tls13_client_encrypt(&client,
                                          ct,
                                          FIO_TLS13_RECORD_HEADER_LEN + chunk +
                                              1 + FIO_TLS13_TAG_LEN,
                                          source + sent,
                                          n);
    FIO_ASSERT(ct_len > 0, "large encrypt failed");

    int dec_len = fio_tls13_server_decrypt(&server,
                                           received + recv_pos,
                                           total - recv_pos,
                                           ct,
                                           (size_t)ct_len);
    FIO_ASSERT(dec_len == (int)n, "large decrypt length wrong");

    sent += n;
    recv_pos += (size_t)dec_len;
  }

  FIO_ASSERT(!FIO_MEMCMP(source, received, total),
             "large transfer data mismatch");

  tls13_test_free(source, total);
  tls13_test_free(received, total);
  tls13_test_free(ct,
                  FIO_TLS13_RECORD_HEADER_LEN + chunk + 1 + FIO_TLS13_TAG_LEN);
  fio_tls13_client_destroy(&client);
  fio_tls13_server_destroy(&server);
  tls13_test_free(server_cert, server_cert_len);
  fio_x509_keypair_clear(&server_kp);
}

/* *****************************************************************************
TLS I/O function registration
***************************************************************************** */
FIO_SFUNC void test_tls13_context_overhead(void) {
  FIO_LOG_INFO("TLS 1.3 global TLS server context overhead: %zu Bytes",
               sizeof(fio___tls13_context_s));
  FIO_LOG_INFO("TLS 1.3 persistent per-connection context overhead: %zu Bytes",
               sizeof(fio___tls13_connection_s) + FIO___TLS13_BUF_TOTAL);
}

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
  FIO_ASSERT(fn.peer_info_next != NULL, "peer_info_next missing");
}

/* *****************************************************************************
SNI host name resolution for client contexts (RFC 6066)
***************************************************************************** */
FIO_SFUNC void test_tls13_sni_from_tls_object(void) {
  fio_io_functions_s fn = fio_tls13_io_functions();

  /* an explicit SNI host name is preferred over certificate names */
  {
    fio_io_tls_s *tls = fio_io_tls_new();
    fio_io_tls_cert_add(tls, "cert-name.example", NULL, NULL, NULL);
    fio_io_tls_sni_set(tls, "explicit.example.com", 20);
    fio___tls13_context_s *ctx =
        (fio___tls13_context_s *)fn.build_context(tls, 1);
    FIO_ASSERT(ctx != NULL, "client build_context failed");
    FIO_ASSERT(!FIO_MEMCMP(ctx->server_name, "explicit.example.com", 21),
               "explicit SNI should be preferred, got: %s",
               ctx->server_name);
    fio___tls13_free_context_task(ctx, NULL);
    fio_io_tls_free(tls);
  }

  /* legacy fallback: the first certificate name is used when no SNI was set */
  {
    fio_io_tls_s *tls = fio_io_tls_new();
    fio_io_tls_cert_add(tls, "cert-name.example", NULL, NULL, NULL);
    fio___tls13_context_s *ctx =
        (fio___tls13_context_s *)fn.build_context(tls, 1);
    FIO_ASSERT(ctx != NULL, "client build_context failed");
    FIO_ASSERT(!FIO_MEMCMP(ctx->server_name, "cert-name.example", 18),
               "certificate name fallback failed, got: %s",
               ctx->server_name);
    fio___tls13_free_context_task(ctx, NULL);
    fio_io_tls_free(tls);
  }

  /* fio_io_connect identity: an IP literal replaces a stale SNI host name and
   * is kept for verification (the ClientHello omits it from SNI). */
  {
    fio_io_tls_s *tls = fio_io_tls_new();
    fio_io_tls_sni_set(tls, "stale.example.com", 17);
    fio___io_tls_peer_name_set(tls, "10.0.0.1", 8);
    fio___tls13_context_s *ctx =
        (fio___tls13_context_s *)fn.build_context(tls, 1);
    FIO_ASSERT(ctx && !FIO_MEMCMP(ctx->server_name, "10.0.0.1", 9),
               "URL identity must replace a stale SNI host name");
    fio___tls13_free_context_task(ctx, NULL);
    fio_io_tls_free(tls);

    uint8_t random[32] = {7}, pk[32] = {9}, buf[1024];
    int none = fio_tls13_build_client_hello(buf, 1024, random, NULL, pk, 0, 0);
    int v4 = fio_tls13_build_client_hello(buf, 1024, random, "10.0.0.1", pk,
                                          0, 0);
    int v6 = fio_tls13_build_client_hello(buf, 1024, random, "::1", pk, 0, 0);
    int dns = fio_tls13_build_client_hello(buf, 1024, random, "a.example", pk,
                                           0, 0);
    FIO_ASSERT(none > 0 && v4 == none && v6 == none && dns > none,
               "ClientHello SNI must carry DNS names only (%d %d %d %d)",
               none, v4, v6, dns);
  }

  /* IP literals are ignored by fio_io_tls_sni_set, so no SNI is sent */
  {
    fio_io_tls_s *tls = fio_io_tls_new();
    fio_io_tls_sni_set(tls, "192.168.0.1", 11);
    FIO_ASSERT(fio_keystr_buf(&tls->sni).len == 0,
               "IP literal must not be stored as SNI");
    fio___tls13_context_s *ctx =
        (fio___tls13_context_s *)fn.build_context(tls, 1);
    FIO_ASSERT(ctx != NULL, "client build_context failed");
    FIO_ASSERT(ctx->server_name[0] == 0,
               "no SNI should be sent for IP literals, got: %s",
               ctx->server_name);
    fio___tls13_free_context_task(ctx, NULL);
    fio_io_tls_free(tls);
  }
}

/* *****************************************************************************
Client certificate trust verification (mTLS)
***************************************************************************** */

/** Server requires a client certificate AND trusts it: handshake succeeds. */
FIO_SFUNC void test_tls13_handshake_client_cert_trusted(void) {
  uint8_t *server_cert = NULL;
  size_t server_cert_len = 0;
  fio_x509_keypair_s server_kp;
  fio_tls13_server_s server;
  fio_ubuf_info_s server_chain[1];
  tls13_test_init_server(&server,
                         &server_cert,
                         &server_cert_len,
                         &server_kp,
                         server_chain);

  fio_tls13_client_s client;
  fio_tls13_client_init(&client, "localhost");
  fio_tls13_client_skip_verification(&client, 1);

  uint8_t *client_cert = NULL;
  size_t client_cert_len = 0;
  fio_x509_keypair_s client_kp;
  FIO_ASSERT(tls13_test_make_p256_cert(&client_cert,
                                       &client_cert_len,
                                       &client_kp) == 0,
             "client cert generation failed");

  /* Trust the client's (self-signed) certificate as its own anchor. */
  const uint8_t *trust_roots[1] = {client_cert};
  const size_t trust_lens[1] = {client_cert_len};
  fio_x509_trust_store_s trust = {.roots = trust_roots,
                                  .root_lens = trust_lens,
                                  .root_count = 1};
  fio_tls13_server_set_trust_store(&server, &trust);

  FIO_ASSERT(tls13_test_run_handshake(&client,
                                      &server,
                                      1,
                                      &client_kp,
                                      client_cert,
                                      client_cert_len),
             "trusted client-cert handshake failed");
  FIO_ASSERT(fio_tls13_server_client_cert_received(&server),
             "server did not receive client certificate");
  FIO_ASSERT(fio_tls13_server_client_cert_verified(&server),
             "server did not verify trusted client certificate");

  fio_tls13_client_destroy(&client);
  fio_tls13_server_destroy(&server);
  tls13_test_free(server_cert, server_cert_len);
  tls13_test_free(client_cert, client_cert_len);
  fio_x509_keypair_clear(&server_kp);
  fio_x509_keypair_clear(&client_kp);
}

/** Server requires a client certificate but does NOT trust it: fails. */
FIO_SFUNC void test_tls13_handshake_client_cert_untrusted(void) {
  uint8_t *server_cert = NULL;
  size_t server_cert_len = 0;
  fio_x509_keypair_s server_kp;
  fio_tls13_server_s server;
  fio_ubuf_info_s server_chain[1];
  tls13_test_init_server(&server,
                         &server_cert,
                         &server_cert_len,
                         &server_kp,
                         server_chain);

  fio_tls13_client_s client;
  fio_tls13_client_init(&client, "localhost");
  fio_tls13_client_skip_verification(&client, 1);

  uint8_t *client_cert = NULL;
  size_t client_cert_len = 0;
  fio_x509_keypair_s client_kp;
  FIO_ASSERT(tls13_test_make_p256_cert(&client_cert,
                                       &client_cert_len,
                                       &client_kp) == 0,
             "client cert generation failed");

  /* Trust store holds a DIFFERENT certificate — client must be rejected. */
  uint8_t *other_cert = NULL;
  size_t other_cert_len = 0;
  fio_x509_keypair_s other_kp;
  FIO_ASSERT(
      tls13_test_make_p256_cert(&other_cert, &other_cert_len, &other_kp) == 0,
      "other cert generation failed");
  const uint8_t *trust_roots[1] = {other_cert};
  const size_t trust_lens[1] = {other_cert_len};
  fio_x509_trust_store_s trust = {.roots = trust_roots,
                                  .root_lens = trust_lens,
                                  .root_count = 1};
  fio_tls13_server_set_trust_store(&server, &trust);

  FIO_ASSERT(!tls13_test_run_handshake(&client,
                                       &server,
                                       1,
                                       &client_kp,
                                       client_cert,
                                       client_cert_len),
             "untrusted client certificate must not complete the handshake");
  FIO_ASSERT(!fio_tls13_server_client_cert_verified(&server),
             "untrusted client certificate must not be marked verified");

  fio_tls13_client_destroy(&client);
  fio_tls13_server_destroy(&server);
  tls13_test_free(server_cert, server_cert_len);
  tls13_test_free(client_cert, client_cert_len);
  tls13_test_free(other_cert, other_cert_len);
  fio_x509_keypair_clear(&server_kp);
  fio_x509_keypair_clear(&client_kp);
  fio_x509_keypair_clear(&other_kp);
}

/** Client verifies the server certificate against its trust store. */
FIO_SFUNC void test_tls13_client_verifies_server(void) {
  uint8_t *server_cert = NULL;
  size_t server_cert_len = 0;
  fio_x509_keypair_s server_kp;
  fio_tls13_server_s server;
  fio_ubuf_info_s server_chain[1];
  tls13_test_init_server(&server,
                         &server_cert,
                         &server_cert_len,
                         &server_kp,
                         server_chain);

  /* Trusting the server's certificate: handshake verifies successfully. */
  const uint8_t *trust_roots[1] = {server_cert};
  const size_t trust_lens[1] = {server_cert_len};
  fio_x509_trust_store_s trust = {.roots = trust_roots,
                                  .root_lens = trust_lens,
                                  .root_count = 1};

  fio_tls13_client_s client;
  fio_tls13_client_init(&client, "localhost");
  fio_tls13_client_set_trust_store(&client, &trust);

  FIO_ASSERT(tls13_test_run_handshake(&client, &server, 0, NULL, NULL, 0),
             "handshake with trusted server cert failed");
  FIO_ASSERT(fio_tls13_client_is_cert_verified(&client),
             "client should report the server certificate verified");
  fio_tls13_client_destroy(&client);
  fio_tls13_server_destroy(&server);

  /* NOT trusting the server's certificate: client must abort. */
  uint8_t *other_cert = NULL;
  size_t other_cert_len = 0;
  fio_x509_keypair_s other_kp;
  FIO_ASSERT(
      tls13_test_make_p256_cert(&other_cert, &other_cert_len, &other_kp) == 0,
      "other cert generation failed");
  fio_tls13_server_s server2;
  fio_ubuf_info_s server_chain2[1];
  tls13_test_free(server_cert, server_cert_len); /* re-generated below */
  tls13_test_init_server(&server2,
                         &server_cert,
                         &server_cert_len,
                         &server_kp,
                         server_chain2);

  const uint8_t *bad_roots[1] = {other_cert};
  const size_t bad_lens[1] = {other_cert_len};
  fio_x509_trust_store_s bad_trust = {.roots = bad_roots,
                                      .root_lens = bad_lens,
                                      .root_count = 1};

  fio_tls13_client_s client2;
  fio_tls13_client_init(&client2, "localhost");
  fio_tls13_client_set_trust_store(&client2, &bad_trust);

  FIO_ASSERT(!tls13_test_run_handshake(&client2, &server2, 0, NULL, NULL, 0),
             "untrusted server certificate must not complete the handshake");
  FIO_ASSERT(!fio_tls13_client_is_cert_verified(&client2),
             "client must not report an untrusted certificate as verified");

  fio_tls13_client_destroy(&client2);
  fio_tls13_server_destroy(&server2);

  /* Trusted certificate, wrong identity (IP literal without an IP SAN): the
   * client must abort and must not send the IP literal as SNI (RFC 6066). */
  fio_tls13_server_s server3;
  fio_ubuf_info_s server_chain3[1];
  tls13_test_free(server_cert, server_cert_len); /* re-generated below */
  tls13_test_init_server(&server3,
                         &server_cert,
                         &server_cert_len,
                         &server_kp,
                         server_chain3);
  fio_tls13_client_s client3;
  const uint8_t *trust3_roots[1] = {server_cert};
  const size_t trust3_lens[1] = {server_cert_len};
  fio_x509_trust_store_s trust3 = {.roots = trust3_roots,
                                   .root_lens = trust3_lens,
                                   .root_count = 1};
  fio_tls13_client_init(&client3, "10.0.0.1");
  fio_tls13_client_set_trust_store(&client3, &trust3);
  FIO_ASSERT(!tls13_test_run_handshake(&client3, &server3, 0, NULL, NULL, 0),
             "trusted certificate lacking the IP SAN must be rejected");
  FIO_ASSERT(!fio_tls13_client_is_cert_verified(&client3),
             "client must not verify a certificate for another identity");
  FIO_ASSERT(!server3.peer_sni[0], "IP literal must not be sent as SNI");
  fio_tls13_client_destroy(&client3);
  fio_tls13_server_destroy(&server3);
  /* control: the same trusted certificate verifies for its own host name */
  fio_tls13_server_init(&server3);
  fio_tls13_server_set_cert_chain(&server3, server_chain3, 1);
  fio_tls13_server_set_private_key(
      &server3,
      FIO_UBUF_INFO2(server_kp.secret_key, server_kp.secret_key_len),
      FIO_TLS13_SIGNATURE_ECDSA_SECP256R1_SHA256);
  FIO_MEMCPY(server3.credentials.public_key, server_kp.public_key, 65);
  fio_tls13_client_init(&client3, "localhost");
  fio_tls13_client_set_trust_store(&client3, &trust3);
  FIO_ASSERT(tls13_test_run_handshake(&client3, &server3, 0, NULL, NULL, 0) &&
                 fio_tls13_client_is_cert_verified(&client3),
             "trusted certificate should verify for its own host name");
  fio_tls13_client_destroy(&client3);
  fio_tls13_server_destroy(&server3);

  tls13_test_free(server_cert, server_cert_len);
  tls13_test_free(other_cert, other_cert_len);
  fio_x509_keypair_clear(&server_kp);
  fio_x509_keypair_clear(&other_kp);
}

/* *****************************************************************************
IO write path helpers: a connected 405 connection pair over a socketpair
***************************************************************************** */

typedef struct {
  fio___tls13_context_s cctx, sctx;
  fio___tls13_connection_s *conn_c;
  fio___tls13_connection_s *conn_s;
  uint8_t *server_cert;
  size_t server_cert_len;
  fio_x509_keypair_s server_kp;
  fio_ubuf_info_s server_chain[1];
  fio_socket_i fds[2]; /* fds[0]: server writes, fds[1]: client reads */
} tls13_test_pair_s;

FIO_SFUNC void tls13_test_pair_init(tls13_test_pair_s *p) {
  FIO_MEMSET(p, 0, sizeof(*p));
  p->cctx.is_client = 1;
  /* stack contexts: hold the owner reference so connections never free them */
  p->cctx.ref = p->sctx.ref = 1;
  p->conn_c = fio___tls13_connection_new(&p->cctx);
  p->conn_s = fio___tls13_connection_new(&p->sctx);
  FIO_ASSERT(p->conn_c && p->conn_s, "connection allocation failed");
  tls13_test_init_server(&p->conn_s->state.server,
                         &p->server_cert,
                         &p->server_cert_len,
                         &p->server_kp,
                         p->server_chain);
  fio_tls13_client_init(&p->conn_c->state.client, "localhost");
  fio_tls13_client_skip_verification(&p->conn_c->state.client, 1);
  FIO_ASSERT(tls13_test_run_handshake(&p->conn_c->state.client,
                                      &p->conn_s->state.server,
                                      0,
                                      NULL,
                                      NULL,
                                      0),
             "connection pair handshake failed");
  p->conn_c->handshake_complete = 1;
  p->conn_s->handshake_complete = 1;
  p->fds[0] = p->fds[1] = FIO_SOCKET_INVALID;
  FIO_ASSERT(!fio_sock_socketpair(p->fds), "socketpair failed");
  fio_sock_set_non_block(p->fds[0]);
  fio_sock_set_non_block(p->fds[1]);
}

FIO_SFUNC void tls13_test_pair_destroy(tls13_test_pair_s *p) {
  fio_sock_close(p->fds[0]);
  fio_sock_close(p->fds[1]);
  fio___tls13_connection_free(p->conn_c);
  fio___tls13_connection_free(p->conn_s);
  tls13_test_free(p->server_cert, p->server_cert_len);
  fio_x509_keypair_clear(&p->server_kp);
}

typedef struct {
  uint8_t wire[96 * 1024];
  size_t wire_len;
  size_t records;
  size_t key_update_records; /* handshake records (0 app bytes) */
  size_t first_app_record;   /* record index of first app data */
  size_t alerts;             /* raw mode: alert records received */
  size_t alert_record;       /* raw mode: record index of the last alert */
  uint8_t alert[2];          /* raw mode: last alert (level, description) */
  uint8_t raw; /* 1: decrypt with fio_tls13_record_decrypt, keep alerts */
  uint8_t *pt;
  size_t pt_len;
} tls13_test_ku_rx_s;

/* Flushes the server's queue, reads the wire and decrypts complete records on
 * the client until `expect` plaintext bytes arrived and all queued output was
 * received. Returns 0 on success. */
FIO_SFUNC int tls13_test_ku_pump(tls13_test_pair_s *p,
                                 tls13_test_ku_rx_s *rx,
                                 size_t expect) {
  for (size_t round = 0; round < 100000; ++round) {
    if (p->conn_s->out_buf_len)
      fio___tls13_out_flush(p->fds[0], p->conn_s);
    ssize_t r = fio_sock_read(p->fds[1],
                              rx->wire + rx->wire_len,
                              sizeof(rx->wire) - rx->wire_len);
    if (r > 0)
      rx->wire_len += (size_t)r;
    for (;;) { /* decrypt complete records */
      if (rx->wire_len < FIO_TLS13_RECORD_HEADER_LEN)
        break;
      size_t rec_len = FIO_TLS13_RECORD_HEADER_LEN +
                       (((size_t)rx->wire[3] << 8) | (size_t)rx->wire[4]);
      if (rx->wire_len < rec_len)
        break;
      int n;
      if (rx->raw) {
        fio_tls13_content_type_e ct;
        n = fio_tls13_record_decrypt(rx->pt + rx->pt_len,
                                     FIO_TLS13_MAX_PLAINTEXT_LEN + 256,
                                     &ct,
                                     rx->wire,
                                     rec_len,
                                     &p->conn_c->state.client.server_app_keys);
        if (n >= 0 && ct == FIO_TLS13_CONTENT_ALERT) {
          if (n != 2)
            return -1;
          FIO_MEMCPY(rx->alert, rx->pt + rx->pt_len, 2);
          rx->alert_record = rx->records;
          ++rx->alerts;
          n = 0;
        } else if (n >= 0 && ct != FIO_TLS13_CONTENT_APPLICATION_DATA)
          return -1;
      } else {
        n = fio_tls13_client_decrypt(&p->conn_c->state.client,
                                     rx->pt + rx->pt_len,
                                     FIO_TLS13_MAX_PLAINTEXT_LEN + 256,
                                     rx->wire,
                                     rec_len);
        if (n == 0)
          ++rx->key_update_records;
      }
      if (n < 0)
        return -1;
      if (n > 0 && rx->pt_len == 0)
        rx->first_app_record = rx->records;
      rx->pt_len += (size_t)n;
      ++rx->records;
      FIO_MEMMOVE(rx->wire, rx->wire + rec_len, rx->wire_len - rec_len);
      rx->wire_len -= rec_len;
    }
    if (rx->pt_len >= expect && !p->conn_s->out_buf_len && !rx->wire_len)
      return rx->pt_len == expect ? 0 : -1;
  }
  return -1;
}

/* *****************************************************************************
IO write path: pending KeyUpdate response (RFC 8446 4.6.3)
***************************************************************************** */

FIO_SFUNC void test_tls13_write_key_update_pending(void) {
  tls13_test_pair_s p;
  tls13_test_pair_init(&p);
  fio___tls13_connection_s *conn_s = p.conn_s;
  fio_socket_i *fds = p.fds;

  const size_t big_len = 3 * FIO_TLS13_MAX_PLAINTEXT_LEN + 1234;
  const size_t pt_cap = big_len + 2 * FIO_TLS13_MAX_PLAINTEXT_LEN + 512;
  uint8_t *msg = (uint8_t *)tls13_test_alloc(big_len);
  tls13_test_ku_rx_s *rx = (tls13_test_ku_rx_s *)tls13_test_alloc(sizeof(*rx));
  uint8_t *pt = (uint8_t *)tls13_test_alloc(pt_cap);
  FIO_ASSERT(msg && rx && pt, "allocation failed");
  for (size_t i = 0; i < big_len; ++i)
    msg[i] = (uint8_t)(i * 31 + 7);

  { /* small write: KeyUpdate response precedes the data record */
    FIO_MEMSET(rx, 0, sizeof(*rx));
    rx->pt = pt;
    conn_s->state.server.key_update_pending = 1;
    FIO_ASSERT(fio___tls13_write(fds[0], msg, 100, conn_s) == 100,
               "write with pending KeyUpdate should succeed");
    FIO_ASSERT(!conn_s->state.server.key_update_pending,
               "pending KeyUpdate should be cleared after write");
    FIO_ASSERT(!tls13_test_ku_pump(&p, rx, 100),
               "client failed to receive KeyUpdate + data");
    FIO_ASSERT(rx->key_update_records == 1 && rx->first_app_record == 1 &&
                   rx->records == 2,
               "KeyUpdate response must be the first record (records=%zu)",
               rx->records);
    FIO_ASSERT(!FIO_MEMCMP(pt, msg, 100), "small write data mismatch");
  }

  { /* multi-record write: first chunk leaves room for the response */
    FIO_MEMSET(rx, 0, sizeof(*rx));
    rx->pt = pt;
    conn_s->state.server.key_update_pending = 1;
    size_t sent = 0;
    for (size_t guard = 0; sent < big_len && guard < 1000; ++guard) {
      ssize_t w = fio___tls13_write(fds[0], msg + sent, big_len - sent, conn_s);
      if (w > 0)
        sent += (size_t)w;
      else
        FIO_ASSERT(errno == EWOULDBLOCK || errno == EAGAIN,
                   "large write failed unexpectedly");
      FIO_ASSERT(!conn_s->state.server.key_update_pending,
                 "pending KeyUpdate should be cleared by the first write");
      FIO_ASSERT(!tls13_test_ku_pump(&p, rx, sent),
                 "large write: client failed to receive data");
    }
    FIO_ASSERT(sent == big_len, "large write did not complete");
    FIO_ASSERT(rx->pt_len == big_len,
               "large write: client got %zu of %zu",
               rx->pt_len,
               big_len);
    FIO_ASSERT(rx->key_update_records == 1 && rx->first_app_record == 1 &&
                   rx->records > 2,
               "large write: KeyUpdate response must lead (records=%zu)",
               rx->records);
    FIO_ASSERT(!FIO_MEMCMP(pt, msg, big_len), "large write data mismatch");
  }

  { /* response failure: connection error, nothing sent under stale keys */
    conn_s->state.server.key_update_pending = 1;
    uint8_t saved = conn_s->state.server.server_app_keys.cipher_type;
    conn_s->state.server.server_app_keys.cipher_type = 99;
    for (int attempt = 0; attempt < 2; ++attempt) {
      errno = 0;
      FIO_ASSERT(fio___tls13_write(fds[0], msg, 100, conn_s) == -1,
                 "write must fail when the KeyUpdate response fails");
      FIO_ASSERT(errno == ECONNRESET,
                 "KeyUpdate failure must be a connection error (errno=%d)",
                 errno);
      FIO_ASSERT(conn_s->state.server.key_update_pending,
                 "failed KeyUpdate response must stay pending");
      FIO_ASSERT(!conn_s->out_buf_len, "nothing may be queued on failure");
    }
    uint8_t tmp[64];
    FIO_ASSERT(fio_sock_read(fds[1], tmp, sizeof(tmp)) <= 0,
               "no bytes may reach the wire on KeyUpdate failure");
    conn_s->state.server.server_app_keys.cipher_type = saved;
  }

  tls13_test_free(pt, pt_cap);
  tls13_test_free(rx, sizeof(*rx));
  tls13_test_free(msg, big_len);
  tls13_test_pair_destroy(&p);
}

/* *****************************************************************************
IO finish: close_notify is queued behind pending output
***************************************************************************** */

FIO_SFUNC void test_tls13_finish_close_notify_order(void) {
  const size_t big_len = 5 * FIO_TLS13_MAX_PLAINTEXT_LEN;
  const size_t pt_cap = big_len + 2 * FIO_TLS13_MAX_PLAINTEXT_LEN + 512;
  uint8_t *msg = (uint8_t *)tls13_test_alloc(big_len);
  tls13_test_ku_rx_s *rx = (tls13_test_ku_rx_s *)tls13_test_alloc(sizeof(*rx));
  uint8_t *pt = (uint8_t *)tls13_test_alloc(pt_cap);
  FIO_ASSERT(msg && rx && pt, "allocation failed");
  for (size_t i = 0; i < big_len; ++i)
    msg[i] = (uint8_t)(i * 13 + 5);
  /* writes to an invalid socket fail, leaving records queued in out_buf */
  const fio_socket_i dead = FIO_SOCKET_INVALID;

  { /* queued data, then finish: data records first, close_notify last */
    tls13_test_pair_s p;
    tls13_test_pair_init(&p);
    FIO_MEMSET(rx, 0, sizeof(*rx));
    rx->pt = pt;
    rx->raw = 1;
    FIO_ASSERT(fio___tls13_write(dead, msg, 100, p.conn_s) == 100,
               "queued write should be accepted");
    size_t queued = p.conn_s->out_buf_len;
    FIO_ASSERT(queued, "write should leave the record queued");
    fio___tls13_finish(dead, p.conn_s);
    FIO_ASSERT(p.conn_s->out_buf_len ==
                   queued + FIO___TLS13_CLOSE_NOTIFY_RECORD_LEN,
               "close_notify must be appended to the queue");
    /* the IO layer relies on flush != 0 while output is queued (402) */
    FIO_ASSERT(fio___tls13_flush(dead, p.conn_s) != 0,
               "flush must report pending output");
    FIO_ASSERT(!tls13_test_ku_pump(&p, rx, 100), "receive after finish failed");
    FIO_ASSERT(fio___tls13_flush(p.fds[0], p.conn_s) == 0,
               "flush must return 0 once all output was sent");
    FIO_ASSERT(rx->records == 2 && rx->alerts == 1 && rx->alert_record == 1,
               "close_notify must follow the data (records=%zu)",
               rx->records);
    FIO_ASSERT(rx->alert[0] == 1 && rx->alert[1] == 0,
               "expected a warning close_notify alert");
    FIO_ASSERT(!FIO_MEMCMP(pt, msg, 100), "data before close_notify mismatch");
    tls13_test_pair_destroy(&p);
  }

  { /* full queue: skip while no room; compact once a prefix was sent */
    tls13_test_pair_s p;
    tls13_test_pair_init(&p);
    FIO_MEMSET(rx, 0, sizeof(*rx));
    rx->pt = pt;
    rx->raw = 1;
    ssize_t w = fio___tls13_write(dead, msg, big_len, p.conn_s);
    FIO_ASSERT(w > 0, "queued large write should be accepted");
    FIO_ASSERT(p.conn_s->out_buf_cap - p.conn_s->out_buf_len <
                   FIO___TLS13_CLOSE_NOTIFY_RECORD_LEN,
               "test setup: expected a full output queue");
    size_t queued = p.conn_s->out_buf_len;
    fio___tls13_finish(dead, p.conn_s);
    FIO_ASSERT(p.conn_s->out_buf_len == queued && !p.conn_s->out_buf_sent,
               "full queue: close_notify must be skipped, data preserved");
    /* pretend the first record was sent: hand it to the receiver directly */
    size_t first = FIO_TLS13_RECORD_HEADER_LEN +
                   (((size_t)p.conn_s->out_buf[3] << 8) |
                    (size_t)p.conn_s->out_buf[4]);
    FIO_MEMCPY(rx->wire, p.conn_s->out_buf, first);
    rx->wire_len = first;
    p.conn_s->out_buf_sent = first;
    fio___tls13_finish(dead, p.conn_s);
    FIO_ASSERT(!p.conn_s->out_buf_sent &&
                   p.conn_s->out_buf_len == queued - first +
                                                FIO___TLS13_CLOSE_NOTIFY_RECORD_LEN,
               "close_notify must be queued after compacting the sent prefix");
    FIO_ASSERT(!tls13_test_ku_pump(&p, rx, (size_t)w),
               "receive after compacted finish failed");
    FIO_ASSERT(rx->alerts == 1 && rx->alert_record == rx->records - 1,
               "close_notify must be the last record (records=%zu)",
               rx->records);
    FIO_ASSERT(!FIO_MEMCMP(pt, msg, (size_t)w), "compacted data mismatch");
    tls13_test_pair_destroy(&p);
  }

  { /* encryption failure: alert skipped, queue intact, no abort */
    tls13_test_pair_s p;
    tls13_test_pair_init(&p);
    FIO_MEMSET(rx, 0, sizeof(*rx));
    rx->pt = pt;
    rx->raw = 1;
    FIO_ASSERT(fio___tls13_write(dead, msg, 100, p.conn_s) == 100,
               "queued write should be accepted");
    size_t queued = p.conn_s->out_buf_len;
    uint8_t saved = p.conn_s->state.server.server_app_keys.cipher_type;
    p.conn_s->state.server.server_app_keys.cipher_type = 99;
    fio___tls13_finish(dead, p.conn_s);
    p.conn_s->state.server.server_app_keys.cipher_type = saved;
    FIO_ASSERT(p.conn_s->out_buf_len == queued,
               "failed close_notify must not change the queue");
    FIO_ASSERT(!tls13_test_ku_pump(&p, rx, 100), "receive after skip failed");
    FIO_ASSERT(rx->records == 1 && !rx->alerts,
               "no alert may be sent when encryption fails");
    tls13_test_pair_destroy(&p);
  }

  { /* handshake incomplete: no alert, pending handshake bytes still flushed */
    tls13_test_pair_s p;
    tls13_test_pair_init(&p);
    p.conn_s->handshake_complete = 0;
    fio___tls13_finish(p.fds[0], p.conn_s);
    FIO_ASSERT(!p.conn_s->out_buf_len, "no close_notify before handshake");
    uint8_t tmp[64];
    FIO_ASSERT(fio_sock_read(p.fds[1], tmp, sizeof(tmp)) <= 0,
               "nothing may be written before the handshake completes");
    tls13_test_pair_destroy(&p);
  }

  tls13_test_free(pt, pt_cap);
  tls13_test_free(rx, sizeof(*rx));
  tls13_test_free(msg, big_len);
}

/* *****************************************************************************
IO read path hardening: retention bounds, empty records, malformed handshakes
***************************************************************************** */

/* Writes all of `buf` to a non-blocking socket (the peer buffer must fit). */
FIO_SFUNC void tls13_test_write_all(fio_socket_i fd,
                                    const void *buf,
                                    size_t len) {
  const uint8_t *p = (const uint8_t *)buf;
  for (size_t tries = 0; len && tries < 100000; ++tries) {
    ssize_t w = fio_sock_write(fd, p, len);
    if (w > 0) {
      p += w;
      len -= (size_t)w;
    }
  }
  FIO_ASSERT(!len, "test socket write failed");
}

FIO_SFUNC void test_tls13_read_hardening(void) {
  const size_t big = 4 * FIO___TLS13_RECV_BUF_CAP;
  uint8_t *wire = (uint8_t *)tls13_test_alloc(big);
  uint8_t *out = (uint8_t *)tls13_test_alloc(big);
  FIO_ASSERT(wire && out, "allocation failed");

  { /* valid record + failing records in one batch: bounded retention */
    tls13_test_pair_s p;
    tls13_test_pair_init(&p);
    int sz = 64 * 1024 * 4;
    setsockopt(p.fds[0], SOL_SOCKET, SO_SNDBUF, (char *)&sz, sizeof(sz));
    setsockopt(p.fds[1], SOL_SOCKET, SO_RCVBUF, (char *)&sz, sizeof(sz));
    int n = fio_tls13_server_encrypt(&p.conn_s->state.server,
                                     wire,
                                     big,
                                     (const uint8_t *)"A",
                                     1);
    FIO_ASSERT(n > 0, "encrypt failed");
    size_t len = (size_t)n;
    /* three max-size application records that fail authentication */
    for (size_t i = 0; i < 3; ++i) {
      uint8_t *r = wire + len;
      FIO_MEMSET(r, 0x5A, FIO___TLS13_RECV_BUF_CAP);
      r[0] = FIO_TLS13_CONTENT_APPLICATION_DATA;
      r[1] = 3;
      r[2] = 3;
      r[3] = (uint8_t)(FIO_TLS13_MAX_CIPHERTEXT_LEN >> 8);
      r[4] = (uint8_t)(FIO_TLS13_MAX_CIPHERTEXT_LEN & 0xFF);
      len += FIO___TLS13_RECV_BUF_CAP;
    }
    tls13_test_write_all(p.fds[0], wire, len);
    ssize_t r = fio___tls13_read(p.fds[1], out, big, p.conn_c);
    FIO_ASSERT(r == 1 && out[0] == 'A', "valid plaintext should be delivered");
    FIO_ASSERT(p.conn_c->recv_buf_len <= FIO___TLS13_RECV_BUF_CAP,
               "retained ciphertext must fit recv_buf");
    errno = 0;
    r = fio___tls13_read(p.fds[1], out, big, p.conn_c);
    FIO_ASSERT(r == -1 && errno == ECONNRESET,
               "the failing record should surface as an error");
    tls13_test_pair_destroy(&p);
  }

  { /* fragmented empty record + large read: not EOF */
    tls13_test_pair_s p;
    tls13_test_pair_init(&p);
    int n = fio_tls13_server_encrypt(&p.conn_s->state.server,
                                     wire,
                                     big,
                                     (const uint8_t *)"",
                                     0);
    FIO_ASSERT(n > 3, "empty record encrypt failed");
    tls13_test_write_all(p.fds[0], wire, 3);
    errno = 0;
    ssize_t r = fio___tls13_read(p.fds[1], out, big, p.conn_c);
    FIO_ASSERT(r == -1 && (errno == EWOULDBLOCK || errno == EAGAIN),
               "partial record should wait for more data");
    FIO_ASSERT(p.conn_c->recv_buf_len == 3, "fragment should be retained");
    tls13_test_write_all(p.fds[0], wire + 3, (size_t)n - 3);
    errno = 0;
    r = fio___tls13_read(p.fds[1], out, big, p.conn_c);
    FIO_ASSERT(r == -1 && (errno == EWOULDBLOCK || errno == EAGAIN),
               "empty record must not read as EOF (%zd)",
               r);
    /* data still flows afterwards */
    n = fio_tls13_server_encrypt(&p.conn_s->state.server,
                                 wire,
                                 big,
                                 (const uint8_t *)"ok",
                                 2);
    tls13_test_write_all(p.fds[0], wire, (size_t)n);
    r = fio___tls13_read(p.fds[1], out, big, p.conn_c);
    FIO_ASSERT(r == 2 && !FIO_MEMCMP(out, "ok", 2), "data after empty record");
    tls13_test_pair_destroy(&p);
  }

  { /* malformed complete handshake record: error, never a spin */
    static const uint8_t bad[][5] = {
        {0x99, 3, 3, 0, 0},       /* invalid content type */
        {0x16, 3, 3, 0xFF, 0xFF}, /* oversized length */
    };
    for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); ++i) {
      fio___tls13_context_s ctx;
      FIO_MEMSET(&ctx, 0, sizeof(ctx));
      ctx.ref = 1;
      fio___tls13_connection_s *c = fio___tls13_connection_new(&ctx);
      FIO_ASSERT(c, "connection allocation failed");
      fio_tls13_server_init(&c->state.server);
      fio_socket_i fds[2];
      FIO_ASSERT(!fio_sock_socketpair(fds), "socketpair failed");
      fio_sock_set_non_block(fds[0]);
      fio_sock_set_non_block(fds[1]);
      tls13_test_write_all(fds[0], bad[i], 5);
      errno = 0;
      ssize_t r = fio___tls13_read(fds[1], out, big, c);
      FIO_ASSERT(r == -1 && errno == ECONNRESET,
                 "malformed handshake record %zu should fail",
                 i);
      fio_sock_close(fds[0]);
      fio_sock_close(fds[1]);
      fio___tls13_connection_free(c);
    }
  }

  tls13_test_free(out, big);
  tls13_test_free(wire, big);
}

static size_t tls13_test_alpn_hits;
FIO_SFUNC void tls13_test_alpn_on_h2(fio_io_s *io) {
  ++tls13_test_alpn_hits;
  (void)io;
}

/* ALPN: both roles collect the registered names; negotiation dispatches. */
FIO_SFUNC void test_tls13_alpn_dispatch(void) {
  fio_io_functions_s fn = fio_tls13_io_functions();
  fio_io_tls_s *tls = fio_io_tls_new();
  fio_io_tls_alpn_add(tls, "h2", tls13_test_alpn_on_h2);
  fio_io_tls_alpn_add(tls, "http/1.1", NULL);
  fio___tls13_context_s *ctx =
      (fio___tls13_context_s *)fn.build_context(tls, 1);
  FIO_ASSERT(ctx, "client build_context failed");
  FIO_ASSERT(!FIO_MEMCMP(ctx->alpn_protocols, "h2,http/1.1", 12),
             "client should offer registered ALPN names, got: %s",
             ctx->alpn_protocols);
  fio___tls13_connection_s *c = fio___tls13_connection_new(ctx);
  FIO_ASSERT(c, "connection allocation failed");
  tls13_test_alpn_hits = 0;
  fio___tls13_alpn_dispatch(c, NULL);
  fio___tls13_alpn_dispatch(c, "spdy/3");
  FIO_ASSERT(!tls13_test_alpn_hits, "no callback without a registered match");
  fio___tls13_alpn_dispatch(c, "h2");
  FIO_ASSERT(tls13_test_alpn_hits == 1, "negotiated ALPN callback missing");
  fio_tls13_client_init(&c->state.client, NULL);
  fio___tls13_connection_free(c);
  fio___tls13_free_context_task(ctx, NULL);
  fio_io_tls_free(tls);
}

/* *****************************************************************************
Peer information iteration (fio___tls13_peer_info_next)
***************************************************************************** */

FIO_SFUNC void test_tls13_peer_info_next(void) {
  uint8_t *cert = NULL, *cert2 = NULL;
  size_t cert_len = 0, cert2_len = 0;
  fio_x509_keypair_s kp, kp2;
  FIO_ASSERT(tls13_test_make_p256_cert(&cert, &cert_len, &kp) == 0,
             "cert generation failed");
  FIO_ASSERT(tls13_test_make_p256_cert(&cert2, &cert2_len, &kp2) == 0,
             "second cert generation failed");

  /* Fabricate a minimal server-side connection holding a 2-certificate
   * client certificate chain (leaf first) */
  fio___tls13_connection_s *conn = (fio___tls13_connection_s *)
      FIO_MEM_REALLOC(NULL, 0, sizeof(*conn) + FIO___TLS13_BUF_TOTAL, 0);
  FIO_ASSERT(conn != NULL, "connection allocation failed");
  FIO_MEMSET(conn, 0, sizeof(*conn) + FIO___TLS13_BUF_TOTAL);
  conn->is_client = 0;
  conn->handshake_complete = 1;
  conn->state.server.peer_auth.chain.certs[0].buf = cert;
  conn->state.server.peer_auth.chain.certs[0].len = cert_len;
  conn->state.server.peer_auth.chain.certs[1].buf = cert2;
  conn->state.server.peer_auth.chain.certs[1].len = cert2_len;
  conn->state.server.peer_auth.chain.count = 2;
  conn->state.server.peer_auth.verified = 1;

  fio_x509_cert_s info;

  /* NULL dest is an error — there is no hidden reset channel */
  FIO_ASSERT(fio___tls13_peer_info_next(-1, NULL, conn) == -1,
             "NULL dest should return -1");

  /* A zeroed dest starts a new loop at the leaf certificate */
  FIO_MEMSET(&info, 0, sizeof(info));
  FIO_ASSERT(fio___tls13_peer_info_next(-1, &info, conn) == 0,
             "peer_info_next failed for first certificate");
  FIO_ASSERT(info.chain_index == 0, "leaf should have chain_index 0");
  FIO_ASSERT(info.verified == 1, "verified flag lost");
  FIO_ASSERT(info.der.buf == cert && info.der.len == cert_len,
             "DER should reference the stored certificate (zero-copy)");
  FIO_ASSERT(info.cn.buf != NULL && info.cn.len == 9 &&
                 !FIO_MEMCMP(info.cn.buf, "localhost", 9),
             "subject CN mismatch");
  FIO_ASSERT(info.key_algo == FIO_X509_KEY_ECDSA_P256, "key type mismatch");
  FIO_ASSERT(info.pubkey.ecdsa.point.buf != NULL &&
                 info.pubkey.ecdsa.point.len == 65,
             "public key missing");
  fio_u256 expected_fp = fio_sha256(cert, cert_len);
  FIO_ASSERT(!FIO_MEMCMP(info.fingerprint, expected_fp.u8, 32),
             "fingerprint should equal SHA-256 of DER");

  /* The next call steps forward using the data in `dest` alone */
  FIO_ASSERT(fio___tls13_peer_info_next(-1, &info, conn) == 0,
             "peer_info_next failed for second certificate");
  FIO_ASSERT(info.chain_index == 1, "second cert should have chain_index 1");
  FIO_ASSERT(info.der.buf == cert2 && info.der.len == cert2_len,
             "second DER should reference the second stored certificate");

  /* Iteration ends with -1 */
  FIO_ASSERT(fio___tls13_peer_info_next(-1, &info, conn) == -1,
             "iteration should end after the last certificate");

  /* Restarting a loop only requires zeroing the struct */
  FIO_MEMSET(&info, 0, sizeof(info));
  FIO_ASSERT(fio___tls13_peer_info_next(-1, &info, conn) == 0 &&
                 info.chain_index == 0 && info.der.buf == cert,
             "zeroed dest should restart the loop at the leaf");

  /* Interleaved loops on the same connection do not interfere */
  fio_x509_cert_s a, b;
  FIO_MEMSET(&a, 0, sizeof(a));
  FIO_MEMSET(&b, 0, sizeof(b));
  FIO_ASSERT(fio___tls13_peer_info_next(-1, &a, conn) == 0 &&
                 a.chain_index == 0,
             "loop A failed at leaf");
  FIO_ASSERT(fio___tls13_peer_info_next(-1, &b, conn) == 0 &&
                 b.chain_index == 0,
             "loop B failed at leaf");
  FIO_ASSERT(fio___tls13_peer_info_next(-1, &a, conn) == 0 &&
                 a.chain_index == 1 && a.der.buf == cert2,
             "loop A should step to the second certificate");
  FIO_ASSERT(b.chain_index == 0 && b.der.buf == cert,
             "loop B state must be untouched by loop A");
  FIO_ASSERT(fio___tls13_peer_info_next(-1, &b, conn) == 0 &&
                 b.chain_index == 1 && b.der.buf == cert2,
             "loop B should step to the second certificate");
  FIO_ASSERT(fio___tls13_peer_info_next(-1, &a, conn) == -1,
             "loop A should end after the last certificate");
  FIO_ASSERT(fio___tls13_peer_info_next(-1, &b, conn) == -1,
             "loop B should end after the last certificate");

  /* Iteration is capped at 128 certificates (deep-nesting / DoS guard) */
  FIO_MEMSET(&info, 0, sizeof(info));
  info.der.buf = cert; /* non-NULL: continue from a (forged) position */
  info.chain_index = 127;
  FIO_ASSERT(fio___tls13_peer_info_next(-1, &info, conn) == -1,
             "iteration must stop at the 128-certificate cap");
  info.chain_index = 200;
  FIO_ASSERT(fio___tls13_peer_info_next(-1, &info, conn) == -1,
             "chain_index >= 128 must return -1");

  /* Skipped (insecure) verification must never be reported as verified */
  FIO_MEMSET(conn, 0, sizeof(*conn) + FIO___TLS13_BUF_TOTAL);
  conn->is_client = 1;
  conn->handshake_complete = 1;
  conn->state.client.peer_chain.certs[0].buf = cert;
  conn->state.client.peer_chain.certs[0].len = cert_len;
  conn->state.client.peer_chain.count = 1;
  conn->state.client.cert_verified = 1;  /* set by the skip path */
  conn->state.client.chain_verified = 1; /* set by the skip path */
  conn->state.client.skip_cert_verify = 1;
  FIO_MEMSET(&info, 0, sizeof(info));
  FIO_ASSERT(fio___tls13_peer_info_next(-1, &info, conn) == 0,
             "peer_info_next failed for client-side certificate");
  FIO_ASSERT(info.verified == 0,
             "skipped verification must not be reported as verified");
  FIO_ASSERT(info.chain_index == 0,
             "client-side leaf should have chain_index 0");

  FIO_MEM_FREE(conn, sizeof(*conn) + FIO___TLS13_BUF_TOTAL);
  tls13_test_free(cert, cert_len);
  tls13_test_free(cert2, cert2_len);
  fio_x509_keypair_clear(&kp);
  fio_x509_keypair_clear(&kp2);
}

/* *****************************************************************************
Server PEM loading: P-256 key without an embedded public key
***************************************************************************** */
FIO_SFUNC int tls13_test_write_file(const char *path, const char *data) {
  FILE *f = fopen(path, "wb");
  if (!f)
    return -1;
  size_t len = strlen(data);
  int r = (fwrite(data, 1, len, f) == len) ? 0 : -1;
  fclose(f);
  return r;
}

FIO_SFUNC void test_tls13_server_pem_p256_derived_pubkey(void) {
#if defined(H___FIO_PEM___H) && defined(H___FIO_X509___H)
  /* RFC 6979 A.2.5 key: SEC1 ECPrivateKey without the optional publicKey. */
  static const char key_pem[] =
      "-----BEGIN EC PRIVATE KEY-----\n"
      "MDECAQEEIMmvqdhFunUWa1whV2ex1pNOUMPbNuibEnuKYis"
      "SD2choAoGCCqGSM49\nAwEH\n"
      "-----END EC PRIVATE KEY-----\n";
  /* Same key with d = n (out of range), also without a public key. */
  static const char bad_key_pem[] = "-----BEGIN EC PRIVATE KEY-----\n"
                                    "MDECAQEEIP////8AAAAA//////////+85vqtpxeeh"
                                    "PO5ysL8YyVRoAoGCCqGSM49\nAwEH\n"
                                    "-----END EC PRIVATE KEY-----\n";
  /* RFC 6979 key with its (matching) public key embedded. */
  static const char match_key_pem[] =
      "-----BEGIN EC PRIVATE KEY-----\n"
      "MHcCAQEEIMmvqdhFunUWa1whV2ex1pNOUMPbNuibEnuKYisSD2choAoGCCqGSM49\n"
      "AwEHoUQDQgAEYP7UuiVanTHJYet0xjVtaMBJuJI7Yfps5mliLmDyn7Z5A/4QCLi8\n"
      "maQa6elWKLxk8vGyDC1+n1F3o8KU1EYimQ==\n"
      "-----END EC PRIVATE KEY-----\n";
  /* RFC 6979 key with a mismatched public key (last y byte changed). */
  static const char mismatch_key_pem[] =
      "-----BEGIN EC PRIVATE KEY-----\n"
      "MHcCAQEEIMmvqdhFunUWa1whV2ex1pNOUMPbNuibEnuKYisSD2choAoGCCqGSM49\n"
      "AwEHoUQDQgAEYP7UuiVanTHJYet0xjVtaMBJuJI7Yfps5mliLmDyn7Z5A/4QCLi8\n"
      "maQa6elWKLxk8vGyDC1+n1F3o8KU1EYimA==\n"
      "-----END EC PRIVATE KEY-----\n";
  /* Self-signed certificate for the RFC 6979 key (CN=localhost). */
  static const char cert_pem[] =
      "-----BEGIN CERTIFICATE-----\n"
      "MIIBfzCCASWgAwIBAgIUK/1tHBB863GXaRh+wrcAj5nYAcEwCgYIKoZIzj0EAwIw\n"
      "FDESMBAGA1UEAwwJbG9jYWxob3N0MCAXDTI2MTAyODA0MjcwMFoYDzIxMjYxMDA0\n"
      "MDQyNzAwWjAUMRIwEAYDVQQDDAlsb2NhbGhvc3QwWTATBgcqhkjOPQIBBggqhkjO\n"
      "PQMBBwNCAARg/tS6JVqdMclh63TGNW1owEm4kjth+mzmaWIuYPKftnkD/hAIuLyZ\n"
      "pBrp6VYovGTy8bIMLX6fUXejwpTURiKZo1MwUTAdBgNVHQ4EFgQUGpVpV5vOMpqU\n"
      "LQdpycC1ZDFWNxAwHwYDVR0jBBgwFoAUGpVpV5vOMpqULQdpycC1ZDFWNxAwDwYD\n"
      "VR0TAQH/BAUwAwEB/zAKBggqhkjOPQQDAgNIADBFAiEApfwKkwejlBE+HjZZpphe\n"
      "tncnI3ao17JmvSxsNbcUpQICIAyx+dZuReE1YKy8RL8/+TuL9fAOXOa5JLY6Wi5Y\n"
      "4Gkb\n"
      "-----END CERTIFICATE-----\n";
  static const uint8_t sk[32] = {
      0xc9, 0xaf, 0xa9, 0xd8, 0x45, 0xba, 0x75, 0x16, 0x6b, 0x5c, 0x21,
      0x57, 0x67, 0xb1, 0xd6, 0x93, 0x4e, 0x50, 0xc3, 0xdb, 0x36, 0xe8,
      0x9b, 0x12, 0x7b, 0x8a, 0x62, 0x2b, 0x12, 0x0f, 0x67, 0x21};
  static const uint8_t pk[65] = {
      0x04, 0x60, 0xfe, 0xd4, 0xba, 0x25, 0x5a, 0x9d, 0x31, 0xc9, 0x61,
      0xeb, 0x74, 0xc6, 0x35, 0x6d, 0x68, 0xc0, 0x49, 0xb8, 0x92, 0x3b,
      0x61, 0xfa, 0x6c, 0xe6, 0x69, 0x62, 0x2e, 0x60, 0xf2, 0x9f, 0xb6,
      0x79, 0x03, 0xfe, 0x10, 0x08, 0xb8, 0xbc, 0x99, 0xa4, 0x1a, 0xe9,
      0xe9, 0x56, 0x28, 0xbc, 0x64, 0xf2, 0xf1, 0xb2, 0x0c, 0x2d, 0x7e,
      0x9f, 0x51, 0x77, 0xa3, 0xc2, 0x94, 0xd4, 0x46, 0x22, 0x99};
  struct {
    const char *pem;
    int loads; /* 1: the PEM key is used; 0: the context build fails */
    const char *desc;
  } keys[] = {
      {key_pem, 1, "derived public key"},
      {match_key_pem, 1, "matching embedded public key"},
      {bad_key_pem, 0, "out-of-range scalar"},
      {mismatch_key_pem, 0, "mismatched embedded public key"},
  };
  char cert_path[128], key_path[128];
  snprintf(cert_path,
           sizeof(cert_path),
           "tmp/tls13-p256-cert-%d.pem",
           (int)getpid());
  snprintf(key_path,
           sizeof(key_path),
           "tmp/tls13-p256-key-%d.pem",
           (int)getpid());
  FIO_ASSERT(!tls13_test_write_file(cert_path, cert_pem),
             "failed to write TLS test PEM files (tmp/ folder missing?)");

  fio_io_functions_s fn = fio_tls13_io_functions();
  for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); ++i) {
    FIO_ASSERT(!tls13_test_write_file(key_path, keys[i].pem),
               "failed to write TLS test key PEM");
    fio_io_tls_s *tls = fio_io_tls_new();
    fio_io_tls_cert_add(tls, "localhost", cert_path, key_path, NULL);
    int log_level = FIO_LOG_LEVEL_GET();
    if (!keys[i].loads) /* expected errors: keep test output clean */
      FIO_LOG_LEVEL_SET(FIO_LOG_LEVEL_NONE);
    fio___tls13_context_s *ctx =
        (fio___tls13_context_s *)fn.build_context(tls, 0);
    FIO_LOG_LEVEL_SET(log_level);
    if (!keys[i].loads) { /* fail closed, never a self-signed substitute */
      FIO_ASSERT(!ctx,
                 "invalid P-256 PEM key must fail the context build (%s)",
                 keys[i].desc);
      fio_io_tls_free(tls);
      continue;
    }
    FIO_ASSERT(ctx != NULL, "server build_context failed (%s)", keys[i].desc);
    FIO_ASSERT(ctx->private_key_len == 32, "no key loaded (%s)", keys[i].desc);
    FIO_ASSERT(!FIO_MEMCMP(ctx->private_key, sk, 32) &&
                   !FIO_MEMCMP(ctx->public_key, pk, 65),
               "P-256 PEM key pair should be loaded (%s)",
               keys[i].desc);
    fio___tls13_free_context_task(ctx, NULL);
    fio_io_tls_free(tls);
  }

  /* PEM bundles: an oversized (> 16 KiB DER) block and a malformed block.
   * The chain loader copies DER without X.509 parsing, so all-zero DER is
   * enough to exercise the large-block path. */
  static const char bad_block[] = "-----BEGIN CERTIFICATE-----\n"
                                  "!!!!\n"
                                  "-----END CERTIFICATE-----\n";
  const size_t big_der_len = 20001; /* multiple of 3: no Base64 padding */
  const size_t big_b64_len = big_der_len / 3 * 4;
  const size_t buf_len = sizeof(cert_pem) * 2 + sizeof(bad_block) +
                         big_b64_len + big_b64_len / 64 + 128;
  char *big = (char *)malloc(buf_len);
  FIO_ASSERT_ALLOC(big);
  const char *cert_der_pem = cert_pem;
#define TLS13_TEST_BUILD_BUNDLE(with_big, with_bad, with_second)               \
  do {                                                                         \
    size_t at_ = 0;                                                            \
    at_ += (size_t)snprintf(big + at_, buf_len - at_, "%s", cert_der_pem);     \
    if (with_big) {                                                            \
      at_ += (size_t)snprintf(big + at_,                                       \
                              buf_len - at_,                                   \
                              "-----BEGIN CERTIFICATE-----\n");                \
      for (size_t j_ = 0; j_ < big_b64_len; ++j_) {                            \
        big[at_++] = 'A';                                                      \
        if ((j_ & 63) == 63)                                                   \
          big[at_++] = '\n';                                                   \
      }                                                                        \
      at_ += (size_t)snprintf(big + at_,                                       \
                              buf_len - at_,                                   \
                              "\n-----END CERTIFICATE-----\n");                \
    }                                                                          \
    if (with_bad)                                                              \
      at_ += (size_t)snprintf(big + at_, buf_len - at_, "%s", bad_block);      \
    if (with_second)                                                           \
      at_ += (size_t)snprintf(big + at_, buf_len - at_, "%s", cert_der_pem);   \
    big[at_] = 0;                                                              \
  } while (0)

  FIO_ASSERT(!tls13_test_write_file(key_path, key_pem),
             "failed to write TLS test key PEM");
  { /* server chain with a > 16 KiB block loads completely */
    TLS13_TEST_BUILD_BUNDLE(1, 0, 0);
    FIO_ASSERT(!tls13_test_write_file(cert_path, big), "write failed");
    fio_io_tls_s *tls = fio_io_tls_new();
    fio_io_tls_cert_add(tls, "localhost", cert_path, key_path, NULL);
    fio___tls13_context_s *ctx =
        (fio___tls13_context_s *)fn.build_context(tls, 0);
    FIO_ASSERT(ctx && ctx->cert_chain_count == 2 &&
                   ctx->cert_chain[1].len == big_der_len &&
                   !FIO_MEMCMP(ctx->private_key, sk, 32),
               "server chain with a large PEM block should load fully");
    fio___tls13_free_context_task(ctx, NULL);
    fio_io_tls_free(tls);
  }
  { /* server chain with a malformed block: the context build fails */
    TLS13_TEST_BUILD_BUNDLE(0, 1, 1);
    FIO_ASSERT(!tls13_test_write_file(cert_path, big), "write failed");
    fio_io_tls_s *tls = fio_io_tls_new();
    fio_io_tls_cert_add(tls, "localhost", cert_path, key_path, NULL);
    int log_level = FIO_LOG_LEVEL_GET();
    FIO_LOG_LEVEL_SET(FIO_LOG_LEVEL_NONE);
    void *ctx = fn.build_context(tls, 0);
    FIO_LOG_LEVEL_SET(log_level);
    FIO_ASSERT(!ctx, "a partial server chain must fail the context build");
    fio_io_tls_free(tls);
  }
  { /* trust bundle: malformed block skipped, the rest (incl. large) loads */
    TLS13_TEST_BUILD_BUNDLE(1, 1, 1);
    FIO_ASSERT(!tls13_test_write_file(cert_path, big), "write failed");
    fio_io_tls_s *tls = fio_io_tls_new();
    fio_io_tls_trust_add(tls, cert_path);
    int log_level = FIO_LOG_LEVEL_GET();
    FIO_LOG_LEVEL_SET(FIO_LOG_LEVEL_NONE);
    fio___tls13_context_s *ctx =
        (fio___tls13_context_s *)fn.build_context(tls, 1);
    FIO_LOG_LEVEL_SET(log_level);
    FIO_ASSERT(ctx && ctx->trust_der_count == 3 &&
                   ctx->trust_store.root_count == 3,
               "trust bundle should skip only the malformed block");
    fio___tls13_free_context_task(ctx, NULL);
    fio_io_tls_free(tls);
  }
  { /* trust bundle with only malformed blocks: build fails cleanly */
    FIO_ASSERT(!tls13_test_write_file(cert_path, bad_block), "write failed");
    fio_io_tls_s *tls = fio_io_tls_new();
    fio_io_tls_trust_add(tls, cert_path);
    int log_level = FIO_LOG_LEVEL_GET();
    FIO_LOG_LEVEL_SET(FIO_LOG_LEVEL_NONE);
    void *ctx = fn.build_context(tls, 1);
    FIO_LOG_LEVEL_SET(log_level);
    FIO_ASSERT(!ctx, "trust bundle without certificates should fail");
    fio_io_tls_free(tls);
  }
#undef TLS13_TEST_BUILD_BUNDLE
  free(big);
  unlink(cert_path);
  unlink(key_path);
#endif
}

/* *****************************************************************************
fio_io_connect `.verify` over loopback (shared with the OpenSSL test)
***************************************************************************** */
#include "tls-connect-verify.h"
#include "tls-pem-load.h" /* PEM load failures fail closed (shared) */

/* *****************************************************************************
Compressed EC points (SEC 1 `02/03 || x`) in certificates and key files
***************************************************************************** */

/** A CA-issued certificate (DER), its CA (DER) and the leaf key pair. */
typedef struct {
  uint8_t leaf[1024], ca[1024];
  size_t leaf_len, ca_len;
  fio_x509_keypair_s kp; /* P-256 only (P-384 keys are OpenSSL-side) */
} tls13_test_ecc_s;

FIO_SFUNC void tls13_test_ecc_load(tls13_test_ecc_s *e,
                                   const char *leaf_pem,
                                   const char *ca_pem,
                                   const char *key_pem) {
  FIO_MEMSET(e, 0, sizeof(*e));
  e->leaf_len = fio_pem_get_certificate_der(e->leaf,
                                            sizeof(e->leaf),
                                            leaf_pem,
                                            FIO_STRLEN(leaf_pem));
  e->ca_len = fio_pem_get_certificate_der(e->ca,
                                          sizeof(e->ca),
                                          ca_pem,
                                          FIO_STRLEN(ca_pem));
  FIO_ASSERT(e->leaf_len && e->ca_len, "compressed EC fixture PEM failed");
  if (!key_pem)
    return;
  fio_pem_private_key_s k;
  FIO_ASSERT(!fio_pem_parse_private_key(&k, key_pem, FIO_STRLEN(key_pem)) &&
                 k.type == FIO_PEM_KEY_ECDSA_P256,
             "compressed EC fixture key failed");
  e->kp.type = FIO_X509_KEYPAIR_P256;
  FIO_MEMCPY(e->kp.secret_key, k.ecdsa_p256.private_key, 32);
  e->kp.secret_key_len = 32;
  e->kp.public_key_len = 65;
  FIO_ASSERT(!fio_p256_public_key(e->kp.public_key, e->kp.secret_key),
             "P-256 public key derivation failed");
  fio_pem_private_key_clear(&k);
}

/** Runs fio___tls13_key_matches_leaf for a PEM key against a DER leaf. */
FIO_SFUNC int tls13_test_key_matches(const uint8_t *leaf,
                                     size_t leaf_len,
                                     const char *key_pem,
                                     const uint8_t *override_sk) {
  fio_ubuf_info_s chain[1] = {FIO_UBUF_INFO2((uint8_t *)leaf, leaf_len)};
  fio___tls13_context_s ctx = {0};
  ctx.cert_chain = chain;
  ctx.cert_chain_count = 1;
  fio_pem_private_key_s k;
  FIO_ASSERT(!fio_pem_parse_private_key(&k, key_pem, FIO_STRLEN(key_pem)),
             "test key PEM failed");
  if (override_sk)
    FIO_MEMCPY(k.ecdsa_p256.private_key, override_sk, 32);
  int r = fio___tls13_key_matches_leaf(&ctx, &k);
  fio_pem_private_key_clear(&k);
  return r;
}

/** Key / certificate matching with compressed leaf points (and embedded
 * compressed public keys), as checked when a server context is built. */
FIO_SFUNC void test_tls13_compressed_key_match(void) {
  tls13_test_ecc_s e;
  tls13_test_ecc_load(&e,
                      fio___test_ecc_p256_leaf,
                      fio___test_ecc_p256_ca,
                      fio___test_ecc_p256_key);
  FIO_ASSERT(e.leaf[0] == 0x30 && (e.kp.public_key[64] & 1),
             "fixture: the leaf point should have an odd y (prefix 03)");
  FIO_ASSERT(!tls13_test_key_matches(e.leaf,
                                     e.leaf_len,
                                     fio___test_ecc_p256_key,
                                     NULL) &&
                 !tls13_test_key_matches(e.leaf,
                                         e.leaf_len,
                                         fio___test_ecc_p256_key_pkcs8,
                                         NULL),
             "key should match its compressed P-256 leaf (SEC1 / PKCS#8)");
  FIO_ASSERT(tls13_test_key_matches(e.leaf,
                                    e.leaf_len,
                                    fio___test_tls_pem_p256b_key,
                                    NULL),
             "another P-256 key must not match a compressed leaf");
  FIO_ASSERT(
      tls13_test_key_matches(e.ca, e.ca_len, fio___test_ecc_p256_key, NULL),
      "the leaf key must not match the (compressed) CA certificate");
  { /* n - d: the negated point -Q has the same x and the other y parity */
    static const uint8_t n[32] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xBC, 0xE6, 0xFA, 0xAD, 0xA7, 0x17,
        0x9E, 0x84, 0xF3, 0xB9, 0xCA, 0xC2, 0xFC, 0x63, 0x25, 0x51};
    uint8_t neg[32], pub[65];
    unsigned borrow = 0;
    for (size_t i = 32; i--;) {
      unsigned t = (unsigned)n[i] - e.kp.secret_key[i] - borrow;
      neg[i] = (uint8_t)t;
      borrow = (t >> 8) & 1;
    }
    FIO_ASSERT(!borrow && !fio_p256_public_key(pub, neg) &&
                   !FIO_MEMCMP(pub + 1, e.kp.public_key + 1, 32) &&
                   (pub[64] & 1) != (e.kp.public_key[64] & 1),
               "n - d should give the same x with the other y parity");
    FIO_ASSERT(tls13_test_key_matches(e.leaf,
                                      e.leaf_len,
                                      fio___test_ecc_p256_key,
                                      neg),
               "the negated key (same x, other parity) must not match");
    fio_secure_zero(neg, sizeof(neg));
  }
  { /* uncompressed leaf: unchanged behavior */
    uint8_t der[1024];
    size_t len =
        fio_pem_get_certificate_der(der,
                                    sizeof(der),
                                    fio___test_tls_pem_cert,
                                    FIO_STRLEN(fio___test_tls_pem_cert));
    FIO_ASSERT(len, "uncompressed fixture PEM failed");
    FIO_ASSERT(!tls13_test_key_matches(der, len, fio___test_tls_pem_key, NULL),
               "key should match its uncompressed P-256 leaf");
    FIO_ASSERT(tls13_test_key_matches(der, len, fio___test_ecc_p256_key, NULL),
               "a compressed-key file must not match another leaf");
  }
  { /* the point comparison helper (embedded public keys use it too) */
    uint8_t c[65];
    const uint8_t *q = e.kp.public_key;
    c[0] = (uint8_t)(0x02 | (q[64] & 1));
    FIO_MEMCPY(c + 1, q + 1, 32);
    FIO_ASSERT(fio___tls13_p256_point_eq(q, q, 65) &&
                   fio___tls13_p256_point_eq(q, c, 33),
               "P-256 point comparison should accept both encodings");
    FIO_ASSERT(!fio___tls13_p256_point_eq(q, c, 32) &&
                   !fio___tls13_p256_point_eq(q, c, 0) &&
                   !fio___tls13_p256_point_eq(q, q, 64) &&
                   !fio___tls13_p256_point_eq(q, q, 33),
               "P-256 point comparison: wrong lengths / encodings must fail");
    c[0] ^= 1;
    FIO_ASSERT(!fio___tls13_p256_point_eq(q, c, 33),
               "P-256 point comparison: the other parity must fail");
    c[0] = 0x04; /* hybrid / wrong prefix with a matching x */
    FIO_ASSERT(!fio___tls13_p256_point_eq(q, c, 33),
               "P-256 point comparison: a 04 prefix on 33 bytes must fail");
    fio_pem_private_key_s k;
    FIO_ASSERT(!fio_pem_parse_private_key(
                   &k,
                   fio___test_ecc_p256_key_badpub,
                   FIO_STRLEN(fio___test_ecc_p256_key_badpub)) &&
                   k.ecdsa_p256.public_key_len == 33 &&
                   !fio___tls13_p256_point_eq(q, k.ecdsa_p256.public_key, 33),
               "badpub: the embedded public key must not match");
    fio_pem_private_key_clear(&k);
  }
  fio_x509_keypair_clear(&e.kp);
  fprintf(stderr, "* TLS 1.3 compressed P-256 key / certificate match: OK\n");
}

/** In-memory native handshakes with a compressed P-256 chain (leaf + CA):
 * the client verifies the server, the server verifies a client certificate. */
FIO_SFUNC void test_tls13_compressed_handshake(void) {
  tls13_test_ecc_s e, other;
  tls13_test_ecc_load(&e,
                      fio___test_ecc_p256_leaf,
                      fio___test_ecc_p256_ca,
                      fio___test_ecc_p256_key);
  tls13_test_ecc_load(&other,
                      fio___test_ecc_p384_leaf,
                      fio___test_ecc_p384_ca,
                      NULL);
  fio_ubuf_info_s chain[2] = {FIO_UBUF_INFO2(e.leaf, e.leaf_len),
                              FIO_UBUF_INFO2(e.ca, e.ca_len)};
  const uint8_t *roots[1] = {e.ca};
  const size_t root_lens[1] = {e.ca_len};
  fio_x509_trust_store_s trust = {.roots = roots,
                                  .root_lens = root_lens,
                                  .root_count = 1};
  const uint8_t *bad_roots[1] = {other.ca};
  const size_t bad_lens[1] = {other.ca_len};
  fio_x509_trust_store_s bad_trust = {.roots = bad_roots,
                                      .root_lens = bad_lens,
                                      .root_count = 1};
  for (int round = 0; round < 4; ++round) {
    /* 0: verified server; 1: + verified client cert (mTLS);
     * 2: client trusts another CA; 3: server trusts another CA (mTLS) */
    fio_tls13_server_s server;
    fio_tls13_client_s client;
    fio_tls13_server_init(&server);
    fio_tls13_server_set_cert_chain(&server, chain, 2);
    fio_tls13_server_set_private_key(
        &server,
        FIO_UBUF_INFO2(e.kp.secret_key, e.kp.secret_key_len),
        FIO_TLS13_SIGNATURE_ECDSA_SECP256R1_SHA256);
    FIO_MEMCPY(server.credentials.public_key, e.kp.public_key, 65);
    fio_tls13_client_init(&client, "localhost");
    fio_tls13_client_set_trust_store(&client, round == 2 ? &bad_trust : &trust);
    if (round & 1)
      fio_tls13_server_set_trust_store(&server,
                                       round == 3 ? &bad_trust : &trust);
    int ok = tls13_test_run_handshake(&client,
                                      &server,
                                      round & 1,
                                      &e.kp,
                                      e.leaf,
                                      e.leaf_len);
    if (round < 2) {
      FIO_ASSERT(ok && fio_tls13_client_is_cert_verified(&client),
                 "compressed P-256 chain: the client should verify the server "
                 "(round %d)",
                 round);
      FIO_ASSERT(!round || (fio_tls13_server_client_cert_received(&server) &&
                            fio_tls13_server_client_cert_verified(&server)),
                 "compressed P-256 client certificate should verify");
    } else {
      FIO_ASSERT(!ok,
                 "compressed P-256 chain: untrusted %s must fail",
                 round == 2 ? "server" : "client");
    }
    fio_tls13_client_destroy(&client);
    fio_tls13_server_destroy(&server);
  }
  fio_x509_keypair_clear(&e.kp);
  fprintf(stderr, "* TLS 1.3 compressed P-256 chain handshakes: OK\n");
}

/* *****************************************************************************
P-384 keys, client certificate chains and RSA client certificates (native)
***************************************************************************** */

/** A certificate chain (DER, leaf first) and its private key in the TLS 1.3
 * key format (EC / Ed25519 scalar, or the length-prefixed RSA format). */
typedef struct {
  uint8_t der[8][12288];
  fio_ubuf_info_s chain[8];
  size_t count;
  uint8_t key[8 * (4 + FIO_RSA_MAX_BYTES)];
  size_t key_len;
  uint16_t algo;
} tls13_test_cred_s;

/** Loads a chain (`pems`, leaf first) and its key (PEM, any supported type).
 * RSA keys keep e and the CRT values (CRT + blinding when signing). */
FIO_SFUNC void tls13_test_cred_load(tls13_test_cred_s *c,
                                    const char *key_pem,
                                    size_t count,
                                    const char **pems) {
  FIO_MEMSET(c, 0, sizeof(*c));
  FIO_ASSERT(count && count <= 8, "test chain size");
  for (size_t i = 0; i < count; ++i) {
    size_t len = fio_pem_get_certificate_der(c->der[i],
                                             sizeof(c->der[i]),
                                             pems[i],
                                             FIO_STRLEN(pems[i]));
    FIO_ASSERT(len, "test chain PEM %zu failed", i);
    c->chain[i] = FIO_UBUF_INFO2(c->der[i], len);
  }
  c->count = count;
  fio_pem_private_key_s k;
  FIO_ASSERT(!fio_pem_parse_private_key(&k, key_pem, FIO_STRLEN(key_pem)),
             "test key PEM failed");
  switch (k.type) {
  case FIO_PEM_KEY_ECDSA_P256:
    FIO_MEMCPY(c->key, k.ecdsa_p256.private_key, 32);
    c->key_len = 32;
    c->algo = FIO_TLS13_SIGNATURE_ECDSA_SECP256R1_SHA256;
    break;
  case FIO_PEM_KEY_ECDSA_P384:
    FIO_MEMCPY(c->key, k.ecdsa_p384.private_key, 48);
    c->key_len = 48;
    c->algo = FIO_TLS13_SIGNATURE_ECDSA_SECP384R1_SHA384;
    break;
  case FIO_PEM_KEY_ED25519:
    FIO_MEMCPY(c->key, k.ed25519.private_key, 32);
    c->key_len = 32;
    c->algo = FIO_TLS13_SIGNATURE_ED25519;
    break;
  case FIO_PEM_KEY_RSA: {
    const uint8_t *v[8] = {k.rsa.n,
                           k.rsa.d,
                           k.rsa.e,
                           k.rsa.p,
                           k.rsa.q,
                           k.rsa.dP,
                           k.rsa.dQ,
                           k.rsa.qInv};
    const size_t l[8] = {k.rsa.n_len,
                         k.rsa.d_len,
                         k.rsa.e_len,
                         k.rsa.p_len,
                         k.rsa.q_len,
                         k.rsa.dP_len,
                         k.rsa.dQ_len,
                         k.rsa.qInv_len};
    for (size_t i = 0; i < 8; ++i) {
      fio_u2buf32_be(c->key + c->key_len, (uint32_t)l[i]);
      FIO_MEMCPY(c->key + c->key_len + 4, v[i], l[i]);
      c->key_len += 4 + l[i];
    }
    c->algo = FIO_TLS13_SIGNATURE_RSA_PSS_RSAE_SHA256;
    break;
  }
  default: FIO_ASSERT(0, "unexpected test key type");
  }
  fio_pem_private_key_clear(&k);
}

/** A one-root trust store (DER copy of `root_pem`). */
typedef struct {
  uint8_t der[2048];
  const uint8_t *roots[1];
  size_t lens[1];
  fio_x509_trust_store_s store;
} tls13_test_trust_s;

FIO_SFUNC fio_x509_trust_store_s *tls13_test_trust(tls13_test_trust_s *t,
                                                   const char *root_pem) {
  t->lens[0] = fio_pem_get_certificate_der(t->der,
                                           sizeof(t->der),
                                           root_pem,
                                           FIO_STRLEN(root_pem));
  FIO_ASSERT(t->lens[0], "trust root PEM failed");
  t->roots[0] = t->der;
  t->store = (fio_x509_trust_store_s){.roots = t->roots,
                                      .root_lens = t->lens,
                                      .root_count = 1};
  return &t->store;
}

/** Native handshake: `s` serves (the client trusts `client_root`); `c`
 * (optional) is a required client certificate, sent as its first
 * `c_count` certificates (0: the whole chain) and verified against
 * `server_root`. Returns 1 when both ends connect and verify. */
FIO_SFUNC int tls13_test_cred_handshake(tls13_test_cred_s *s,
                                        const char *client_root,
                                        tls13_test_cred_s *c,
                                        size_t c_count,
                                        const char *server_root,
                                        uint16_t *server_scheme) {
  static tls13_test_trust_s ct, st;
  fio_tls13_server_s server;
  fio_tls13_client_s client;
  fio_tls13_server_init(&server);
  fio_tls13_server_set_cert_chain(&server, s->chain, s->count);
  fio_tls13_server_set_private_key(&server,
                                   FIO_UBUF_INFO2(s->key, s->key_len),
                                   (fio_tls13_signature_algo_e)s->algo);
  fio_tls13_client_init(&client, "localhost");
  fio_tls13_client_set_trust_store(&client, tls13_test_trust(&ct, client_root));
  if (c) {
    fio_tls13_server_require_client_cert(&server, 2);
    fio_tls13_server_set_trust_store(&server, tls13_test_trust(&st, server_root));
    fio_tls13_client_set_cert_chain(&client,
                                    c->chain,
                                    c_count ? c_count : c->count,
                                    FIO_UBUF_INFO2(c->key, c->key_len),
                                    (fio_tls13_signature_algo_e)c->algo);
  }
  int ok = tls13_test_run_handshake(&client, &server, 0, NULL, NULL, 0) &&
           fio_tls13_client_is_cert_verified(&client);
  if (ok && c)
    ok = fio_tls13_server_client_cert_received(&server) &&
         fio_tls13_server_client_cert_verified(&server) &&
         server.peer_auth.chain.count == (c_count ? c_count : c->count);
  if (server_scheme)
    *server_scheme = server.signature_algo;
  fio_tls13_client_destroy(&client);
  fio_tls13_server_destroy(&server);
  return ok;
}

/** P-384 certificates (compressed and uncompressed points): native server,
 * native client, both directions of authentication (CertificateVerify 0x0503).
 */
FIO_SFUNC void test_tls13_p384_handshake(void) {
  static tls13_test_cred_s pc, pu;
  tls13_test_cred_load(
      &pc,
      fio___test_ecc_p384_key,
      2,
      (const char *[]){fio___test_ecc_p384_leaf, fio___test_ecc_p384_ca});
  tls13_test_cred_load(
      &pu,
      fio___test_ecc_p384u_key,
      2,
      (const char *[]){fio___test_ecc_p384u_leaf, fio___test_ecc_p384u_ca});
  FIO_ASSERT(pc.algo == FIO_TLS13_SIGNATURE_ECDSA_SECP384R1_SHA384 &&
                 pc.key_len == 48 && pu.key_len == 48,
             "P-384 test keys should load as 48 byte scalars");
  struct {
    tls13_test_cred_s *c;
    const char *ca, *other_ca, *name;
  } t[] = {
      {&pc, fio___test_ecc_p384_ca, fio___test_ecc_p384u_ca, "compressed"},
      {&pu, fio___test_ecc_p384u_ca, fio___test_ecc_p384_ca, "uncompressed"},
  };
  int log_level = FIO_LOG_LEVEL_GET();
  for (size_t i = 0; i < 2; ++i) {
    uint16_t scheme = 0;
    FIO_ASSERT(tls13_test_cred_handshake(t[i].c, t[i].ca, NULL, 0, NULL, &scheme),
               "P-384 (%s) server: the native client should verify it",
               t[i].name);
    FIO_ASSERT(scheme == FIO_TLS13_SIGNATURE_ECDSA_SECP384R1_SHA384,
               "P-384 server should sign ecdsa_secp384r1_sha384 (got 0x%04x)",
               scheme);
    FIO_ASSERT(tls13_test_cred_handshake(t[i].c, t[i].ca, t[i].c, 0, t[i].ca, 0),
               "P-384 (%s) mTLS: the server should verify the client",
               t[i].name);
    FIO_LOG_LEVEL_SET(FIO_LOG_LEVEL_FATAL); /* expected failures */
    FIO_ASSERT(!tls13_test_cred_handshake(t[i].c, t[i].other_ca, 0, 0, 0, 0),
               "P-384 (%s): a client trusting another CA must fail",
               t[i].name);
    FIO_ASSERT(!tls13_test_cred_handshake(t[i].c,
                                          t[i].ca,
                                          t[i].c,
                                          0,
                                          t[i].other_ca,
                                          0),
               "P-384 (%s): a server trusting another CA must reject the "
               "client",
               t[i].name);
    FIO_LOG_LEVEL_SET(log_level);
  }
  /* mixed: P-384 server, P-256 client certificate and the reverse */
  static tls13_test_cred_s p256;
  tls13_test_cred_load(
      &p256,
      fio___test_ecc_p256_key,
      2,
      (const char *[]){fio___test_ecc_p256_leaf, fio___test_ecc_p256_ca});
  FIO_ASSERT(tls13_test_cred_handshake(&pc,
                                       fio___test_ecc_p384_ca,
                                       &p256,
                                       0,
                                       fio___test_ecc_p256_ca,
                                       0) &&
                 tls13_test_cred_handshake(&p256,
                                           fio___test_ecc_p256_ca,
                                           &pu,
                                           0,
                                           fio___test_ecc_p384u_ca,
                                           0),
             "mixed P-256 / P-384 server and client certificates failed");
  fprintf(stderr, "* TLS 1.3 P-384 certificates (server and mTLS): OK\n");
}

/** Client certificate chains (leaf + intermediate, the server trusts only the
 * root) with RSA and P-384 leaves. */
FIO_SFUNC void test_tls13_client_cert_chain(void) {
  static tls13_test_cred_s srv, rsa, p384;
  tls13_test_cred_load(
      &srv,
      fio___test_ecc_p384_key,
      2,
      (const char *[]){fio___test_ecc_p384_leaf, fio___test_ecc_p384_ca});
  tls13_test_cred_load(
      &rsa,
      fio___test_mtls_rsa_key,
      2,
      (const char *[]){fio___test_mtls_rsa_leaf, fio___test_mtls_int});
  tls13_test_cred_load(
      &p384,
      fio___test_mtls_p384_key,
      2,
      (const char *[]){fio___test_mtls_p384_leaf, fio___test_mtls_int});
  FIO_ASSERT(rsa.algo == FIO_TLS13_SIGNATURE_RSA_PSS_RSAE_SHA256,
             "RSA test key should load");
  tls13_test_cred_s *clients[] = {&rsa, &p384};
  const char *names[] = {"RSA", "P-384"};
  int log_level = FIO_LOG_LEVEL_GET();
  for (size_t i = 0; i < 2; ++i) {
    FIO_ASSERT(tls13_test_cred_handshake(&srv,
                                         fio___test_ecc_p384_ca,
                                         clients[i],
                                         0,
                                         fio___test_mtls_root,
                                         0),
               "%s client chain (leaf + intermediate) should verify against "
               "the root",
               names[i]);
    FIO_LOG_LEVEL_SET(FIO_LOG_LEVEL_FATAL); /* expected failures */
    FIO_ASSERT(!tls13_test_cred_handshake(&srv,
                                          fio___test_ecc_p384_ca,
                                          clients[i],
                                          1,
                                          fio___test_mtls_root,
                                          0),
               "%s leaf without its intermediate must not verify",
               names[i]);
    FIO_ASSERT(!tls13_test_cred_handshake(&srv,
                                          fio___test_ecc_p384_ca,
                                          clients[i],
                                          0,
                                          fio___test_ecc_p384_ca,
                                          0),
               "%s client chain: a server trusting another root must fail",
               names[i]);
    FIO_LOG_LEVEL_SET(log_level);
  }
  { /* the RSA leaf also serves (RSA-PSS CertificateVerify from the server) */
    uint16_t scheme = 0;
    FIO_ASSERT(tls13_test_cred_handshake(&rsa,
                                         fio___test_mtls_root,
                                         &p384,
                                         0,
                                         fio___test_mtls_root,
                                         &scheme) &&
                   scheme == FIO_TLS13_SIGNATURE_RSA_PSS_RSAE_SHA256,
               "RSA server chain + P-384 client chain failed");
  }
  { /* set_cert sends the leaf only; set_cert_chain(0) clears it */
    fio_tls13_client_s cl;
    fio_tls13_client_init(&cl, "localhost");
    size_t n = 9;
    fio_tls13_client_set_cert_chain(&cl,
                                    rsa.chain,
                                    2,
                                    FIO_UBUF_INFO2(rsa.key, rsa.key_len),
                                    FIO_TLS13_SIGNATURE_RSA_PSS_RSAE_SHA256);
    FIO_ASSERT(fio___tls13_client_chain(&cl, &n) == rsa.chain && n == 2 &&
                   cl.auth.cert.buf == rsa.chain[0].buf,
               "set_cert_chain should keep the chain and its leaf");
    fio_tls13_client_set_cert(&cl,
                              rsa.chain[0],
                              FIO_UBUF_INFO2(rsa.key, rsa.key_len),
                              FIO_TLS13_SIGNATURE_RSA_PSS_RSAE_SHA256);
    FIO_ASSERT(fio___tls13_client_chain(&cl, &n) == &cl.auth.cert && n == 1,
               "set_cert should send the leaf only");
    fio_tls13_client_set_cert_chain(&cl,
                                    rsa.chain,
                                    0,
                                    FIO_UBUF_INFO2(rsa.key, rsa.key_len),
                                    FIO_TLS13_SIGNATURE_RSA_PSS_RSAE_SHA256);
    FIO_ASSERT(!fio___tls13_client_chain(&cl, &n) && !n,
               "an empty chain should send no certificate");
    fio_tls13_client_destroy(&cl);
  }
  fprintf(stderr, "* TLS 1.3 client certificate chains (RSA, P-384): OK\n");
}

/** Copies the ClientHello message `ch` (with its 4 byte header) into `out`,
 * replacing the signature_algorithms list with `algos`. Returns the length. */
FIO_SFUNC size_t tls13_test_ch_sigalgs(uint8_t *out,
                                       const uint8_t *ch,
                                       size_t ch_len,
                                       const uint16_t *algos,
                                       size_t count) {
  const uint8_t *p = ch + 4 + 2 + 32;
  p += 1 + p[0];                          /* session id */
  p += 2 + ((size_t)p[0] << 8 | p[1]);    /* cipher suites */
  p += 1 + p[0];                          /* compression */
  const uint8_t *ext = p + 2, *end = ch + ch_len;
  size_t head = (size_t)(ext - ch);
  FIO_MEMCPY(out, ch, head);
  uint8_t *o = out + head;
  while (ext + 4 <= end) {
    size_t type = (size_t)ext[0] << 8 | ext[1];
    size_t len = (size_t)ext[2] << 8 | ext[3];
    if (type == FIO_TLS13_EXT_SIGNATURE_ALGORITHMS) {
      fio_u2buf16_be(o, (uint16_t)type);
      fio_u2buf16_be(o + 2, (uint16_t)(2 + 2 * count));
      fio_u2buf16_be(o + 4, (uint16_t)(2 * count));
      o += 6;
      for (size_t i = 0; i < count; ++i, o += 2)
        fio_u2buf16_be(o, algos[i]);
    } else {
      FIO_MEMCPY(o, ext, 4 + len);
      o += 4 + len;
    }
    ext += 4 + len;
  }
  size_t total = (size_t)(o - out);
  fio_u2buf16_be(out + head - 2, (uint16_t)(total - head));
  out[1] = (uint8_t)((total - 4) >> 16);
  out[2] = (uint8_t)((total - 4) >> 8);
  out[3] = (uint8_t)(total - 4);
  return total;
}

/** Signature scheme selection by key type and the peer's list, and peer
 * lists keeping known schemes past unknown ones. */
FIO_SFUNC void test_tls13_sig_select(void) {
  const uint16_t ec256 = FIO_TLS13_SIGNATURE_ECDSA_SECP256R1_SHA256,
                 ec384 = FIO_TLS13_SIGNATURE_ECDSA_SECP384R1_SHA384,
                 ed = FIO_TLS13_SIGNATURE_ED25519,
                 pss256 = FIO_TLS13_SIGNATURE_RSA_PSS_RSAE_SHA256,
                 pss384 = FIO_TLS13_SIGNATURE_RSA_PSS_RSAE_SHA384;
  const uint16_t all[] = {ed, ec256, ec384, pss256, pss384};
  const uint16_t only384[] = {pss384, ec384};
  const uint16_t pkcs1[] = {0x0401, 0x0501, 0x0601};
  FIO_ASSERT(fio___tls13_sig_select(ec384, all, 5) == ec384 &&
                 fio___tls13_sig_select(ec256, all, 5) == ec256 &&
                 fio___tls13_sig_select(ed, all, 5) == ed,
             "EC / Ed25519 keys should get their own scheme");
  FIO_ASSERT(!fio___tls13_sig_select(ec256, only384, 2) &&
                 !fio___tls13_sig_select(ed, only384, 2) &&
                 !fio___tls13_sig_select(ec384, all, 2) &&
                 !fio___tls13_sig_select(ec384, NULL, 5) &&
                 !fio___tls13_sig_select(0, all, 5),
             "EC / Ed25519 keys have exactly one scheme");
  FIO_ASSERT(fio___tls13_sig_select(pss256, all, 5) == pss256 &&
                 fio___tls13_sig_select(pss384, all, 5) == pss384,
             "RSA keys should prefer the configured hash");
  FIO_ASSERT(fio___tls13_sig_select(pss256, only384, 2) == pss384 &&
                 fio___tls13_sig_select(pss384, all + 3, 1) == pss256,
             "RSA keys should fall back to the other PSS hash");
  FIO_ASSERT(!fio___tls13_sig_select(pss256, pkcs1, 3) &&
                 !fio___tls13_sig_select(pss256, all, 3),
             "RSA keys never sign PKCS#1 v1.5 (or a non-RSA scheme)");

  /* peer lists: 20 unknown schemes before known ones (CertificateRequest) */
  uint16_t lst[24];
  for (size_t i = 0; i < 20; ++i)
    lst[i] = (uint16_t)(0x0900 + i); /* e.g., ML-DSA style code points */
  lst[20] = ec384;
  lst[21] = 0x0201; /* rsa_pkcs1_sha1: not known */
  lst[22] = pss256;
  lst[23] = 0x0904;
  {
    uint8_t ctx[1] = {7}, msg[256];
    int n = fio_tls13_build_certificate_request(msg,
                                                sizeof(msg),
                                                FIO_UBUF_INFO2(ctx, 1),
                                                lst,
                                                24);
    fio_tls13_certificate_request_s cr;
    FIO_ASSERT(n > 4 && !fio_tls13_parse_certificate_request(&cr,
                                                             msg + 4,
                                                             (size_t)n - 4),
               "CertificateRequest with a long list should parse");
    FIO_ASSERT(cr.algo_count == 2 && cr.algos[0] == ec384 &&
                   cr.algos[1] == pss256,
               "CertificateRequest should keep the known schemes (%zu)",
               cr.algo_count);
  }
  { /* ClientHello: same filtering, and the server selects 0x0503 */
    uint8_t random[32] = {1}, x25519_pk[32] = {9};
    static uint8_t ch[4096], ch2[4096];
    int n = fio_tls13_build_client_hello(ch,
                                         sizeof(ch),
                                         random,
                                         "localhost",
                                         x25519_pk,
                                         NULL,
                                         0);
    FIO_ASSERT(n > 0, "ClientHello build failed");
    size_t n2 = tls13_test_ch_sigalgs(ch2, ch, (size_t)n, lst, 24);
    static fio___tls13_client_hello_s parsed;
    FIO_ASSERT(!fio___tls13_parse_client_hello(&parsed, ch2 + 4, n2 - 4),
               "rewritten ClientHello should parse");
    FIO_ASSERT(parsed.algo_count == 2 && parsed.algos[0] == ec384 &&
                   parsed.algos[1] == pss256,
               "ClientHello should keep the known schemes (%zu)",
               parsed.algo_count);
  }
  fprintf(stderr, "* TLS 1.3 signature scheme selection: OK\n");
}

/** IO layer: client contexts load a certificate (chain) from PEM files and
 * present it when the server requests one (both contexts via the backend). */
FIO_SFUNC void test_tls13_io_client_cert(void) {
  fio_io_functions_s fn = fio_tls13_io_functions();
  char sc[64], sk[64], root[64], sca[64], mrc[64], mrk[64], mpc[64], mpk[64];
  fio___test_tls_pem_chain(sc,
                           64,
                           "io-sc",
                           fio___test_ecc_p384_leaf,
                           fio___test_ecc_p384_ca);
  fio___test_tls_pem_file(sk, 64, "io-sk", fio___test_ecc_p384_key);
  fio___test_tls_pem_file(sca, 64, "io-sca", fio___test_ecc_p384_ca);
  fio___test_tls_pem_file(root, 64, "io-root", fio___test_mtls_root);
  fio___test_tls_pem_chain(mrc,
                           64,
                           "io-mrc",
                           fio___test_mtls_rsa_leaf,
                           fio___test_mtls_int);
  fio___test_tls_pem_file(mrk, 64, "io-mrk", fio___test_mtls_rsa_key);
  fio___test_tls_pem_chain(mpc,
                           64,
                           "io-mpc",
                           fio___test_mtls_p384_leaf,
                           fio___test_mtls_int);
  fio___test_tls_pem_file(mpk, 64, "io-mpk", fio___test_mtls_p384_key);

  fio_io_tls_s *stls = fio_io_tls_new();
  fio_io_tls_cert_add(stls, "localhost", sc, sk, NULL);
  fio_io_tls_trust_add(stls, root); /* requires client certificates */
  fio___tls13_context_s *sctx =
      (fio___tls13_context_s *)fn.build_context(stls, 0);
  FIO_ASSERT(sctx && sctx->signature_algo ==
                         FIO_TLS13_SIGNATURE_ECDSA_SECP384R1_SHA384 &&
                 sctx->private_key_len == 48,
             "P-384 server context build failed");
  struct {
    const char *name, *cert, *key;
    int ok;
  } t[] = {
      {"localhost", mrc, mrk, 1},     /* RSA leaf + intermediate */
      {"localhost", mpc, mpk, 1},     /* P-384 leaf + intermediate */
      {"localhost", NULL, NULL, 0},   /* name only: no certificate */
      {"other.example", sc, sk, 0},   /* untrusted (another root) */
  };
  int log_level = FIO_LOG_LEVEL_GET();
  for (size_t i = 0; i < sizeof(t) / sizeof(t[0]); ++i) {
    fio_io_tls_s *ctls = fio_io_tls_new();
    fio_io_tls_cert_add(ctls, t[i].name, t[i].cert, t[i].key, NULL);
    fio_io_tls_cert_add(ctls, "unused.example", NULL, NULL, NULL);
    fio_io_tls_trust_add(ctls, sca);
    fio___tls13_context_s *cctx =
        (fio___tls13_context_s *)fn.build_context(ctls, 1);
    FIO_ASSERT(cctx && cctx->creds_count == 2 &&
                   cctx->cert_chain_count == (size_t)(t[i].cert ? 2 : 0),
               "client context build failed (case %zu)",
               i);
    fio___tls13_connection_s *cs = fio___tls13_connection_new(sctx);
    fio___tls13_connection_s *cc = fio___tls13_connection_new(cctx);
    FIO_ASSERT(cs && cc, "connection allocation failed");
    fio___tls13_server_setup(cs);
    fio___tls13_client_setup(cc);
    cc->state.client.server_name = "localhost"; /* SNI / host check */
    FIO_LOG_LEVEL_SET(FIO_LOG_LEVEL_FATAL); /* expected failures */
    int ok = tls13_test_run_handshake(&cc->state.client,
                                      &cs->state.server,
                                      0,
                                      NULL,
                                      NULL,
                                      0);
    FIO_LOG_LEVEL_SET(log_level);
    FIO_ASSERT(ok == t[i].ok &&
                   fio_tls13_server_client_cert_verified(&cs->state.server) ==
                       t[i].ok &&
                   (!ok || cs->state.server.peer_auth.chain.count == 2),
               "IO client certificate case %zu should %s",
               i,
               t[i].ok ? "verify (chain sent)" : "fail");
    fio___tls13_connection_free(cc);
    fio___tls13_connection_free(cs);
    fio___tls13_free_context_task(cctx, NULL);
    fio_io_tls_free(ctls);
  }
  fio___tls13_free_context_task(sctx, NULL);
  fio_io_tls_free(stls);
  unlink(sc);
  unlink(sk);
  unlink(sca);
  unlink(root);
  unlink(mrc);
  unlink(mrk);
  unlink(mpc);
  unlink(mpk);
  fprintf(stderr, "* TLS 1.3 IO client certificates (mTLS, chains): OK\n");
}

/** P-384 key / certificate matching (both leaf point forms) and the P-384
 * point comparison, as checked when a context is built. */
FIO_SFUNC void test_tls13_p384_key_match(void) {
  static tls13_test_cred_s pc, pu;
  tls13_test_cred_load(
      &pc,
      fio___test_ecc_p384_key,
      2,
      (const char *[]){fio___test_ecc_p384_leaf, fio___test_ecc_p384_ca});
  tls13_test_cred_load(
      &pu,
      fio___test_ecc_p384u_key,
      2,
      (const char *[]){fio___test_ecc_p384u_leaf, fio___test_ecc_p384u_ca});
  fio_x509_cert_s leaf;
  FIO_ASSERT(!fio_x509_parse(&leaf, pc.chain[0].buf, pc.chain[0].len) &&
                 leaf.pubkey.ecdsa.point.len == 49,
             "fixture: the compressed P-384 leaf should hold a 49 byte point");
  FIO_ASSERT(!fio_x509_parse(&leaf, pu.chain[0].buf, pu.chain[0].len) &&
                 leaf.pubkey.ecdsa.point.len == 97,
             "fixture: the uncompressed P-384 leaf should hold a 97 byte "
             "point");
#define TLS13_TEST_KM(c, key) tls13_test_key_matches((c).chain[0].buf,       \
                                                     (c).chain[0].len,       \
                                                     key,                    \
                                                     NULL)
  FIO_ASSERT(!TLS13_TEST_KM(pc, fio___test_ecc_p384_key) &&
                 !TLS13_TEST_KM(pc, fio___test_ecc_p384_key_pkcs8) &&
                 !TLS13_TEST_KM(pu, fio___test_ecc_p384u_key) &&
                 !TLS13_TEST_KM(pu, fio___test_ecc_p384u_key_pkcs8),
             "P-384 keys should match their leaf (both point forms)");
  FIO_ASSERT(TLS13_TEST_KM(pc, fio___test_ecc_p384u_key) &&
                 TLS13_TEST_KM(pu, fio___test_ecc_p384_key) &&
                 TLS13_TEST_KM(pu, fio___test_ecc_p384_key_pkcs8),
             "a P-384 key must not match another P-384 leaf");
  FIO_ASSERT(TLS13_TEST_KM(pc, fio___test_ecc_p256_key) &&
                 TLS13_TEST_KM(pu, fio___test_tls_pem_key) &&
                 TLS13_TEST_KM(pc, fio___test_tls_pem_ed_key) &&
                 TLS13_TEST_KM(pc, fio___test_tls_pem_rsa_key),
             "P-256 / Ed25519 / RSA keys must not match a P-384 leaf");
  {
    uint8_t der[1024];
    size_t len = fio_pem_get_certificate_der(der,
                                             sizeof(der),
                                             fio___test_ecc_p256_leaf,
                                             FIO_STRLEN(fio___test_ecc_p256_leaf));
    FIO_ASSERT(len &&
                   tls13_test_key_matches(der,
                                          len,
                                          fio___test_ecc_p384_key,
                                          NULL) &&
                   tls13_test_key_matches(pc.chain[1].buf,
                                          pc.chain[1].len,
                                          fio___test_ecc_p384_key,
                                          NULL),
               "a P-384 key must not match a P-256 leaf (or its CA)");
  }
#undef TLS13_TEST_KM
  { /* the P-384 point comparison (embedded public keys use it too) */
    uint8_t q[97], c[97];
    FIO_ASSERT(!fio_p384_public_key(q, pc.key), "P-384 derivation failed");
    c[0] = (uint8_t)(0x02 | (q[96] & 1));
    FIO_MEMCPY(c + 1, q + 1, 48);
    FIO_ASSERT(fio___tls13_p384_point_eq(q, q, 97) &&
                   fio___tls13_p384_point_eq(q, c, 49) &&
                   fio___tls13_ec_point_eq(q, 48, c, 49),
               "P-384 point comparison should accept both encodings");
    FIO_ASSERT(!fio___tls13_p384_point_eq(q, c, 48) &&
                   !fio___tls13_p384_point_eq(q, c, 0) &&
                   !fio___tls13_p384_point_eq(q, q, 96) &&
                   !fio___tls13_p384_point_eq(q, q, 65) &&
                   !fio___tls13_p384_point_eq(q, c, 33) &&
                   !fio___tls13_p384_point_eq(q, q, 49),
               "P-384 point comparison: wrong lengths / encodings must fail");
    c[0] ^= 1;
    FIO_ASSERT(!fio___tls13_p384_point_eq(q, c, 49),
               "P-384 point comparison: the other parity must fail");
    c[0] = 0x04;
    FIO_ASSERT(!fio___tls13_p384_point_eq(q, c, 49),
               "P-384 point comparison: a 04 prefix on 49 bytes must fail");
    c[0] = (uint8_t)(0x02 | (q[96] & 1));
    c[48] ^= 1; /* last x byte */
    FIO_ASSERT(!fio___tls13_p384_point_eq(q, c, 49),
               "P-384 point comparison: another x must fail");
    FIO_MEMCPY(c, q, 97);
    c[96] ^= 0x80; /* last y byte (uncompressed) */
    FIO_ASSERT(!fio___tls13_p384_point_eq(q, c, 97),
               "P-384 point comparison: another y must fail");
    fio_pem_private_key_s k;
    FIO_ASSERT(!fio_pem_parse_private_key(
                   &k,
                   fio___test_ecc_p384_key_badpub,
                   FIO_STRLEN(fio___test_ecc_p384_key_badpub)) &&
                   k.ecdsa_p384.public_key_len == 49 &&
                   !fio___tls13_p384_point_eq(q, k.ecdsa_p384.public_key, 49),
               "P-384 badpub: the embedded public key must not match");
    fio_pem_private_key_clear(&k);
    FIO_ASSERT(!fio_pem_parse_private_key(
                   &k,
                   fio___test_ecc_p384u_key,
                   FIO_STRLEN(fio___test_ecc_p384u_key)) &&
                   k.ecdsa_p384.public_key_len == 97 &&
                   !fio_p384_public_key(q, k.ecdsa_p384.private_key) &&
                   fio___tls13_p384_point_eq(q, k.ecdsa_p384.public_key, 97),
               "P-384 uncompressed embedded public key should match");
    fio_pem_private_key_clear(&k);
  }
  fprintf(stderr, "* TLS 1.3 P-384 key / certificate match: OK\n");
}

/** RSA keys loaded by the 405 context keep e and the CRT parameters (the TLS
 * layer needs them for CRT signing, blinding and the fault check). */
FIO_SFUNC void test_tls13_rsa_key_crt(void) {
  fio_io_functions_s fn = fio_tls13_io_functions();
  char rc[64], rk[64];
  fio___test_tls_pem_chain(rc,
                           64,
                           "crt-rc",
                           fio___test_mtls_rsa_leaf,
                           fio___test_mtls_int);
  fio___test_tls_pem_file(rk, 64, "crt-rk", fio___test_mtls_rsa_key);
  fio_io_tls_s *tls = fio_io_tls_new();
  fio_io_tls_cert_add(tls, "localhost", rc, rk, NULL);
  fio___tls13_context_s *ctx =
      (fio___tls13_context_s *)fn.build_context(tls, 0);
  FIO_ASSERT(ctx && !ctx->private_key_len && ctx->private_key_ext.len,
             "an RSA key should be stored in the extended key buffer");
  fio_rsa_privkey_s k;
  FIO_ASSERT(!fio___tls13_parse_rsa_private_key(&k,
                                                ctx->private_key_ext.buf,
                                                ctx->private_key_ext.len) &&
                 k.n_len == 256 && k.d_len && k.e_len && k.p_len && k.q_len &&
                 k.dP_len && k.dQ_len && k.qInv_len,
             "RSA key fields: n %zu d %zu e %zu p %zu q %zu dP %zu dQ %zu "
             "qInv %zu",
             k.n_len,
             k.d_len,
             k.e_len,
             k.p_len,
             k.q_len,
             k.dP_len,
             k.dQ_len,
             k.qInv_len);
  /* every field matches the PEM key */
  fio_pem_private_key_s pem;
  FIO_ASSERT(!fio_pem_parse_private_key(&pem,
                                        fio___test_mtls_rsa_key,
                                        FIO_STRLEN(fio___test_mtls_rsa_key)) &&
                 pem.type == FIO_PEM_KEY_RSA,
             "RSA fixture key should parse");
  const uint8_t *v[8] = {k.n, k.d, k.e, k.p, k.q, k.dP, k.dQ, k.qInv};
  const size_t l[8] = {k.n_len,
                       k.d_len,
                       k.e_len,
                       k.p_len,
                       k.q_len,
                       k.dP_len,
                       k.dQ_len,
                       k.qInv_len};
  const uint8_t *pv[8] = {pem.rsa.n,
                          pem.rsa.d,
                          pem.rsa.e,
                          pem.rsa.p,
                          pem.rsa.q,
                          pem.rsa.dP,
                          pem.rsa.dQ,
                          pem.rsa.qInv};
  const size_t pl[8] = {pem.rsa.n_len,
                        pem.rsa.d_len,
                        pem.rsa.e_len,
                        pem.rsa.p_len,
                        pem.rsa.q_len,
                        pem.rsa.dP_len,
                        pem.rsa.dQ_len,
                        pem.rsa.qInv_len};
  for (size_t i = 0; i < 8; ++i)
    FIO_ASSERT(l[i] == pl[i] && !memcmp(v[i], pv[i], l[i]),
               "RSA key field %zu should match the PEM key",
               i);
  fio_pem_private_key_clear(&pem);
  fio___tls13_free_context_task(ctx, NULL);
  fio_io_tls_free(tls);
  unlink(rc);
  unlink(rk);
  fprintf(stderr, "* TLS 1.3 RSA keys keep CRT parameters: OK\n");
}

/* *****************************************************************************
Large certificate chains: handshake flights spanning several records
***************************************************************************** */

/** Counts the records of content type `type` in `buf` (whole records only),
 * returning their count, the first one's length and the largest length. */
FIO_SFUNC size_t tls13_test_records(const uint8_t *buf,
                                    size_t len,
                                    uint8_t type,
                                    size_t *first_len,
                                    size_t *max_len) {
  size_t n = 0, pos = 0;
  *first_len = *max_len = 0;
  while (pos + FIO_TLS13_RECORD_HEADER_LEN <= len) {
    size_t l = ((size_t)buf[pos + 3] << 8) | (size_t)buf[pos + 4];
    if (buf[pos] == type) {
      if (!n++)
        *first_len = l;
      if (l > *max_len)
        *max_len = l;
    }
    pos += FIO_TLS13_RECORD_HEADER_LEN + l;
  }
  FIO_ASSERT(pos == len, "test: flight should hold whole records");
  return n;
}

/** Loads the > 16 KiB chain (RSA-4096 leaf + intermediate, mTLS root). */
FIO_SFUNC void tls13_test_big_load(tls13_test_cred_s *big) {
  fio___test_mtls_big_init();
  tls13_test_cred_load(
      big,
      fio___test_mtls_big_key,
      2,
      (const char *[]){fio___test_mtls_big_leaf, fio___test_mtls_big_int});
  FIO_ASSERT(big->chain[0].len + big->chain[1].len >
                 FIO_TLS13_MAX_PLAINTEXT_LEN,
             "fixture: the big chain should exceed one record");
}

static uint8_t tls13_test_big_so[65536], tls13_test_big_co[65536];

/** Runs the first flights by hand: a server presenting `s` (requiring `c`'s
 * chain when `c` is set) and a client trusting the mTLS root. The server and
 * client outputs (`tls13_test_big_so` / `_co`) are limited to `s_cap` /
 * `c_cap` bytes (0: 64 KiB). Sets each side's flight length, or -1 if that
 * side failed (it must then be in an error state, having output nothing). */
FIO_SFUNC void tls13_test_big_flights(tls13_test_cred_s *s,
                                      tls13_test_cred_s *c,
                                      size_t s_cap,
                                      size_t c_cap,
                                      long *s_len,
                                      long *c_len) {
  static uint8_t ch[4096];
  static tls13_test_trust_s ct, st;
  fio_tls13_server_s server;
  fio_tls13_client_s client;
  fio_tls13_server_init(&server);
  fio_tls13_server_set_cert_chain(&server, s->chain, s->count);
  fio_tls13_server_set_private_key(&server,
                                   FIO_UBUF_INFO2(s->key, s->key_len),
                                   (fio_tls13_signature_algo_e)s->algo);
  fio_tls13_client_init(&client, "localhost");
  fio_tls13_client_set_trust_store(&client,
                                   tls13_test_trust(&ct, fio___test_mtls_root));
  if (c) {
    fio_tls13_server_require_client_cert(&server, 2);
    fio_tls13_server_set_trust_store(
        &server,
        tls13_test_trust(&st, fio___test_mtls_root));
    fio_tls13_client_set_cert_chain(&client,
                                    c->chain,
                                    c->count,
                                    FIO_UBUF_INFO2(c->key, c->key_len),
                                    (fio_tls13_signature_algo_e)c->algo);
  }
  *s_len = *c_len = -1;
  int n = fio_tls13_client_start(&client, ch, sizeof(ch));
  FIO_ASSERT(n > 0, "client start failed");
  size_t out_len = 0;
  int r = fio_tls13_server_process(&server,
                                   ch,
                                   (size_t)n,
                                   tls13_test_big_so,
                                   s_cap ? s_cap : sizeof(tls13_test_big_so),
                                   &out_len);
  if (r < 0) {
    FIO_ASSERT(fio_tls13_server_is_error(&server),
               "a failed server flight should leave an error state");
  } else {
    FIO_ASSERT(r == n && out_len, "server flight expected");
    *s_len = (long)out_len;
    size_t consumed = 0, c_out = 0;
    int k = 0;
    while (consumed < out_len) {
      size_t ol = 0;
      k = fio_tls13_client_process(&client,
                                   tls13_test_big_so + consumed,
                                   out_len - consumed,
                                   tls13_test_big_co,
                                   c_cap ? c_cap : sizeof(tls13_test_big_co),
                                   &ol);
      if (k <= 0)
        break;
      consumed += (size_t)k;
      if (ol)
        c_out = ol;
    }
    if (k > 0) {
      FIO_ASSERT(consumed == out_len && c_out &&
                     fio_tls13_client_is_connected(&client),
                 "client flight expected");
      *c_len = (long)c_out;
    } else {
      FIO_ASSERT(fio_tls13_client_is_error(&client),
                 "a failed client flight should leave an error state");
    }
  }
  fio_tls13_client_destroy(&client);
  fio_tls13_server_destroy(&server);
}

/** A chain larger than one record (16 KiB): server and client flights are
 * split into full records, fit `fio_tls13_flight_size`, fail closed when the
 * output buffer is short, and complete handshakes in both directions. */
FIO_SFUNC void test_tls13_big_chain(void) {
  static tls13_test_cred_s big, mp;
  tls13_test_big_load(&big);
  tls13_test_cred_load(
      &mp,
      fio___test_mtls_p384_key,
      2,
      (const char *[]){fio___test_mtls_p384_leaf, fio___test_mtls_int});
  const size_t full = FIO_TLS13_MAX_PLAINTEXT_LEN + 1 + FIO_TLS13_TAG_LEN;
  long sl, cl;
  size_t first, max, n;
  int log_level = FIO_LOG_LEVEL_GET();
  FIO_LOG_LEVEL_SET(FIO_LOG_LEVEL_FATAL); /* expected failures */

  /* a small chain: one encrypted record each way */
  tls13_test_big_flights(&mp, &mp, 0, 0, &sl, &cl);
  FIO_ASSERT(sl > 0 && cl > 0, "small chain flights failed");
  FIO_ASSERT(tls13_test_records(tls13_test_big_so,
                                (size_t)sl,
                                FIO_TLS13_CONTENT_APPLICATION_DATA,
                                &first,
                                &max) == 1 &&
                 tls13_test_records(tls13_test_big_co,
                                    (size_t)cl,
                                    FIO_TLS13_CONTENT_APPLICATION_DATA,
                                    &first,
                                    &max) == 1,
             "a small flight should be a single record");
  FIO_ASSERT((size_t)sl <= fio_tls13_flight_size(mp.chain, mp.count) &&
                 (size_t)cl <= fio_tls13_flight_size(mp.chain, mp.count),
             "fio_tls13_flight_size should cover a small flight");

  /* the server's flight */
  tls13_test_big_flights(&big, NULL, 0, 0, &sl, &cl);
  FIO_ASSERT(sl > 0 && cl > 0, "big server chain: handshake flights failed");
  n = tls13_test_records(tls13_test_big_so,
                         (size_t)sl,
                         FIO_TLS13_CONTENT_APPLICATION_DATA,
                         &first,
                         &max);
  FIO_ASSERT(n >= 2 && first == full && max == full,
             "big server flight: %zu records (first %zu, largest %zu bytes), "
             "expected full 16 KiB records",
             n,
             first,
             max);
  size_t need = fio_tls13_flight_size(big.chain, big.count);
  FIO_ASSERT((size_t)sl <= need,
             "fio_tls13_flight_size (%zu) should cover the flight (%ld)",
             need,
             sl);
  long exact = sl;
  tls13_test_big_flights(&big, NULL, (size_t)exact - 1, 0, &sl, &cl);
  FIO_ASSERT(sl == -1, "a short server output buffer must fail closed");
  tls13_test_big_flights(&big, NULL, need, 0, &sl, &cl);
  FIO_ASSERT(sl == exact && cl > 0,
             "fio_tls13_flight_size output should be enough for the server");

  /* the client's flight (mTLS) */
  tls13_test_big_flights(&mp, &big, 0, 0, &sl, &cl);
  FIO_ASSERT(sl > 0 && cl > 0, "big client chain: handshake flights failed");
  n = tls13_test_records(tls13_test_big_co,
                         (size_t)cl,
                         FIO_TLS13_CONTENT_APPLICATION_DATA,
                         &first,
                         &max);
  FIO_ASSERT(n >= 2 && first == full && max == full,
             "big client flight: %zu records (first %zu, largest %zu bytes), "
             "expected full 16 KiB records",
             n,
             first,
             max);
  FIO_ASSERT((size_t)cl <= need,
             "fio_tls13_flight_size (%zu) should cover the client flight (%ld)",
             need,
             cl);
  exact = cl;
  tls13_test_big_flights(&mp, &big, 0, (size_t)exact - 1, &sl, &cl);
  FIO_ASSERT(sl > 0 && cl == -1,
             "a short client output buffer must fail closed");
  tls13_test_big_flights(&mp, &big, 0, need, &sl, &cl);
  FIO_ASSERT(cl == exact,
             "fio_tls13_flight_size output should be enough for the client");

  { /* fio_tls13_flight_size charges one more record at the 16 KiB
     * plaintext boundary (a certificate's exact length decides it) */
    static uint8_t blob[16384];
    fio_ubuf_info_s one = FIO_UBUF_INFO2(blob, 14000);
    size_t prev = fio_tls13_flight_size(&one, 1), jumps = 0, steps = 0;
    for (one.len = 14001; one.len <= 16000; ++one.len) {
      size_t cap_now = fio_tls13_flight_size(&one, 1);
      size_t d = cap_now - prev;
      FIO_ASSERT(d == 1 || d == FIO_TLS13_RECORD_HEADER_LEN + 1 +
                                    FIO_TLS13_TAG_LEN + 1,
                 "flight size should grow by a byte or by a record (%zu at "
                 "%zu)",
                 d,
                 one.len);
      if (d > 1)
        ++jumps;
      else
        ++steps;
      prev = cap_now;
    }
    FIO_ASSERT(jumps == 1 && steps == 1999,
               "expected exactly one record boundary (%zu jumps, %zu steps)",
               jumps,
               steps);
  }

  /* complete handshakes: verified chains both ways */
  uint16_t scheme = 0;
  FIO_ASSERT(tls13_test_cred_handshake(&big,
                                       fio___test_mtls_root,
                                       NULL,
                                       0,
                                       NULL,
                                       &scheme) &&
                 scheme == FIO_TLS13_SIGNATURE_RSA_PSS_RSAE_SHA256,
             "big server chain: the native client should verify it");
  FIO_ASSERT(tls13_test_cred_handshake(&mp,
                                       fio___test_mtls_root,
                                       &big,
                                       0,
                                       fio___test_mtls_root,
                                       NULL),
             "big client chain: the native server should verify it");
  FIO_ASSERT(tls13_test_cred_handshake(&big,
                                       fio___test_mtls_root,
                                       &big,
                                       0,
                                       fio___test_mtls_root,
                                       NULL),
             "big chains both ways should verify");
  FIO_ASSERT(!tls13_test_cred_handshake(&big,
                                        fio___test_mtls_root,
                                        &big,
                                        1,
                                        fio___test_mtls_root,
                                        NULL),
             "a big leaf without its intermediate must be rejected");
  FIO_LOG_LEVEL_SET(log_level);
  fprintf(stderr, "* TLS 1.3 chains over 16 KiB (multi-record flights): OK\n");
}

/** Drives two 405 connections over a socketpair (fio___tls13_read / flush,
 * the IO path) until both complete the handshake, then sends a message each
 * way. Returns 1 on success. */
FIO_SFUNC int tls13_test_io_socket_handshake(fio___tls13_connection_s *cs,
                                             fio___tls13_connection_s *cc) {
  fio_socket_i fds[2] = {FIO_SOCKET_INVALID, FIO_SOCKET_INVALID};
  FIO_ASSERT(!fio_sock_socketpair(fds), "socketpair failed");
  fio_sock_set_non_block(fds[0]); /* fds[0]: server, fds[1]: client */
  fio_sock_set_non_block(fds[1]);
  int ch =
      fio_tls13_client_start(&cc->state.client, cc->out_buf, cc->out_buf_cap);
  FIO_ASSERT(ch > 0, "ClientHello failed");
  cc->out_buf_len = (size_t)ch;
  int ok = 0;
  char tmp[64];
  for (size_t round = 0; round < 100000 && !ok; ++round) {
    fio___tls13_out_flush(fds[1], cc);
    fio___tls13_out_flush(fds[0], cs);
    errno = 0;
    if (!cs->handshake_complete &&
        fio___tls13_read(fds[0], tmp, sizeof(tmp), cs) < 0 &&
        errno != EWOULDBLOCK && errno != EAGAIN)
      break;
    errno = 0;
    if (!cc->handshake_complete &&
        fio___tls13_read(fds[1], tmp, sizeof(tmp), cc) < 0 &&
        errno != EWOULDBLOCK && errno != EAGAIN)
      break;
    ok = cs->handshake_complete && cc->handshake_complete && !cs->out_buf_len &&
         !cc->out_buf_len;
  }
  if (!ok)
    fprintf(stderr,
            "IO handshake failed: server %s (alert %d), client %s (alert %d), "
            "errno %d, client recv %zu/%zu, server queue %zu/%zu\n",
            fio_tls13_server_state_name(&cs->state.server),
            (int)cs->state.server.alert_description,
            fio_tls13_client_state_name(&cc->state.client),
            (int)cc->state.client.alert_description,
            errno,
            cc->recv_buf_pos,
            cc->recv_buf_len,
            cs->out_buf_sent,
            cs->out_buf_len);
  if (ok) { /* application data both ways */
    ok = fio___tls13_write(fds[0], "ping", 4, cs) == 4 &&
         fio___tls13_write(fds[1], "pong", 4, cc) == 4;
    size_t got_c = 0, got_s = 0;
    char rc[8], rs[8];
    for (size_t round = 0; ok && round < 100000 && (got_c < 4 || got_s < 4);
         ++round) {
      ssize_t r = fio___tls13_read(fds[1], rc + got_c, 4 - got_c, cc);
      if (r > 0)
        got_c += (size_t)r;
      r = fio___tls13_read(fds[0], rs + got_s, 4 - got_s, cs);
      if (r > 0)
        got_s += (size_t)r;
    }
    ok = ok && got_c == 4 && got_s == 4 && !memcmp(rc, "ping", 4) &&
         !memcmp(rs, "pong", 4);
  }
  fio_sock_close(fds[0]);
  fio_sock_close(fds[1]);
  return ok;
}

/** IO contexts (PEM files) with a > 16 KiB chain on both sides: the 405 layer
 * sizes its handshake output from the chain (no fixed flight buffer). */
FIO_SFUNC void test_tls13_io_big_chain(void) {
  static tls13_test_cred_s big, mp;
  tls13_test_big_load(&big);
  tls13_test_cred_load(
      &mp,
      fio___test_mtls_p384_key,
      2,
      (const char *[]){fio___test_mtls_p384_leaf, fio___test_mtls_int});
  fio_io_functions_s fn = fio_tls13_io_functions();
  char bc[64], bk[64], mc[64], mk[64], root[64];
  fio___test_tls_pem_chain(bc,
                           64,
                           "io-bc",
                           fio___test_mtls_big_leaf,
                           fio___test_mtls_big_int);
  fio___test_tls_pem_file(bk, 64, "io-bk", fio___test_mtls_big_key);
  fio___test_tls_pem_chain(mc,
                           64,
                           "io-mc",
                           fio___test_mtls_p384_leaf,
                           fio___test_mtls_int);
  fio___test_tls_pem_file(mk, 64, "io-mk", fio___test_mtls_p384_key);
  fio___test_tls_pem_file(root, 64, "io-root", fio___test_mtls_root);
  const size_t big_flight = fio_tls13_flight_size(big.chain, big.count);
  const size_t mp_flight = fio_tls13_flight_size(mp.chain, mp.count);
  FIO_ASSERT(big_flight > mp_flight, "test: flight sizes");

  for (int order = 0; order < 2; ++order) {
    /* the big chain is the default certificate, then an SNI one */
    fio_io_tls_s *stls = fio_io_tls_new();
    if (order)
      fio_io_tls_cert_add(stls, "small.example", mc, mk, NULL);
    fio_io_tls_cert_add(stls, "localhost", bc, bk, NULL);
    if (!order)
      fio_io_tls_cert_add(stls, "small.example", mc, mk, NULL);
    fio_io_tls_trust_add(stls, root); /* requires client certificates */
    fio___tls13_context_s *sctx =
        (fio___tls13_context_s *)fn.build_context(stls, 0);
    FIO_ASSERT(sctx && sctx->creds_count == 2 &&
                   sctx->flight_size == big_flight,
               "server context: flight size %zu, expected %zu (order %d)",
               sctx ? sctx->flight_size : 0,
               big_flight,
               order);
    for (int client_big = 0; client_big < 2; ++client_big) {
      fio_io_tls_s *ctls = fio_io_tls_new();
      fio_io_tls_cert_add(ctls,
                          "localhost",
                          client_big ? bc : mc,
                          client_big ? bk : mk,
                          NULL);
      fio_io_tls_trust_add(ctls, root);
      fio___tls13_context_s *cctx =
          (fio___tls13_context_s *)fn.build_context(ctls, 1);
      FIO_ASSERT(cctx &&
                     cctx->flight_size == (client_big ? big_flight : mp_flight),
                 "client context: flight size %zu",
                 cctx ? cctx->flight_size : 0);
      fio___tls13_connection_s *cs = fio___tls13_connection_new(sctx);
      fio___tls13_connection_s *cc = fio___tls13_connection_new(cctx);
      FIO_ASSERT(cs && cc, "connection allocation failed");
      fio___tls13_server_setup(cs);
      fio___tls13_client_setup(cc);
      FIO_ASSERT(tls13_test_io_socket_handshake(cs, cc),
                 "IO handshake with a big server chain failed (order %d, "
                 "big client chain %d)",
                 order,
                 client_big);
      FIO_ASSERT(fio_tls13_client_is_cert_verified(&cc->state.client) &&
                     fio_tls13_server_client_cert_verified(&cs->state.server),
                 "IO big chains should verify both ways");
      FIO_ASSERT(cs->out_buf == cs->out_inline && cc->out_buf == cc->out_inline,
                 "drained output queues should be inline");
      fio___tls13_connection_free(cc);
      fio___tls13_connection_free(cs);
      fio___tls13_free_context_task(cctx, NULL);
      fio_io_tls_free(ctls);
    }
    fio___tls13_free_context_task(sctx, NULL);
    fio_io_tls_free(stls);
  }
  unlink(bc);
  unlink(bk);
  unlink(mc);
  unlink(mk);
  unlink(root);
  fprintf(stderr, "* TLS 1.3 IO chains over 16 KiB: OK\n");
}

/** The 405 wire-output queue grows past its inline buffer only on demand,
 * keeps queued bytes and returns to the inline buffer once drained. */
FIO_SFUNC void test_tls13_io_out_reserve(void) {
  fio___tls13_context_s ctx = {.ref = 1};
  fio___tls13_connection_s *c = fio___tls13_connection_new(&ctx);
  FIO_ASSERT(c && c->out_buf == c->out_inline &&
                 c->out_buf_cap == sizeof(c->out_inline),
             "a new connection should queue output inline");
  FIO_ASSERT(FIO___TLS13_HS_OUT_NEED(&ctx) == FIO___TLS13_HS_OUT_MIN,
             "a context without certificates reserves the minimum");
  ctx.flight_size = FIO___TLS13_HS_OUT_MIN + 1;
  FIO_ASSERT(FIO___TLS13_HS_OUT_NEED(&ctx) == FIO___TLS13_HS_OUT_MIN + 1,
             "a context reserves its largest flight");
  const size_t inline_cap = sizeof(c->out_inline);
  for (size_t i = 0; i < 100; ++i)
    c->out_buf[i] = (uint8_t)i;
  c->out_buf_len = 100;
  c->out_buf_sent = 10;
  FIO_ASSERT(!fio___tls13_out_reserve(c, inline_cap - 100) &&
                 c->out_buf == c->out_inline && c->out_buf_len == 100 &&
                 c->out_buf_sent == 10,
             "a fitting reservation should change nothing");
  FIO_ASSERT(!fio___tls13_out_reserve(c, inline_cap - 90) &&
                 c->out_buf == c->out_inline && c->out_buf_len == 90 &&
                 !c->out_buf_sent && c->out_buf[0] == 10 &&
                 c->out_buf[89] == 99,
             "dropping the sent prefix should make room in place");
  FIO_ASSERT(!fio___tls13_out_reserve(c, inline_cap) &&
                 c->out_buf != c->out_inline &&
                 c->out_buf_cap == 90 + inline_cap && c->out_buf_len == 90 &&
                 c->out_buf[0] == 10 && c->out_buf[89] == 99,
             "a larger reservation should move the queue to an allocation");
  FIO_ASSERT(!fio___tls13_out_reserve(c, inline_cap) &&
                 c->out_buf_cap == 90 + inline_cap,
             "a fitting reservation should keep the allocation");
  fio_socket_i fds[2];
  FIO_ASSERT(!fio_sock_socketpair(fds), "socketpair failed");
  FIO_ASSERT(fio___tls13_out_flush(fds[0], c) == 90 && !c->out_buf_len &&
                 c->out_buf == c->out_inline && c->out_buf_cap == inline_cap,
             "a drained queue should return to the inline buffer");
  fio_sock_close(fds[0]);
  fio_sock_close(fds[1]);
  FIO_ASSERT(!fio___tls13_out_reserve(c, inline_cap + 1) &&
                 c->out_buf != c->out_inline,
             "reservation without queued data");
  fio___tls13_connection_free(c); /* frees the allocation (leak counters) */
  fprintf(stderr, "* TLS 1.3 IO output queue reservation: OK\n");
}

#if HAVE_OPENSSL && FIO_OS_POSIX
/* *****************************************************************************
OpenSSL interoperability with compressed EC points (memory BIOs)
***************************************************************************** */
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>

FIO_SFUNC X509 *tls13_test_ossl_x509(const char *pem) {
  BIO *b = BIO_new_mem_buf(pem, -1);
  X509 *x = b ? PEM_read_bio_X509(b, NULL, NULL, NULL) : NULL;
  BIO_free(b);
  FIO_ASSERT(x, "OpenSSL failed to load a compressed EC certificate");
  return x;
}

/** Moves pending OpenSSL output into a native endpoint and its answer back.
 * Returns 0 on success, -1 if the native endpoint failed. */
FIO_SFUNC int tls13_test_ossl_pump(SSL *ssl, void *native, int is_server) {
  static uint8_t in[65536], out[65536];
  size_t in_len = 0;
  BIO *wb = SSL_get_wbio(ssl);
  for (int n; (n = BIO_read(wb, in + in_len, (int)(sizeof(in) - in_len))) > 0;)
    in_len += (size_t)n;
  if (!in_len)
    return 0;
  int out_len =
      tls13_test_feed(native, is_server, in, in_len, out, sizeof(out));
  if (out_len < 0)
    return -1;
  if (out_len)
    FIO_ASSERT(BIO_write(SSL_get_rbio(ssl), out, out_len) == out_len,
               "BIO_write failed");
  return 0;
}

FIO_SFUNC SSL *tls13_test_ossl_new(SSL_CTX *ctx) {
  SSL *ssl = SSL_new(ctx);
  BIO *rb = BIO_new(BIO_s_mem()), *wb = BIO_new(BIO_s_mem());
  FIO_ASSERT(ssl && rb && wb, "OpenSSL allocation failed");
  SSL_set_bio(ssl, rb, wb);
  return ssl;
}

/** OpenSSL client (verifying `ca_pem`, TLS 1.3) -> native server serving `s`.
 * With `cl` (a chain, leaf first, and its key: `cl[0..2]`, `cl[1]` may be
 * NULL), OpenSSL presents a client certificate that the native server
 * requires and verifies against `server_root`. `ok`: expected outcome; a
 * failure is either side rejecting the other. */
FIO_SFUNC void tls13_test_ossl_client(tls13_test_cred_s *s,
                                      const char *ca_pem,
                                      const char **cl,
                                      const char *server_root,
                                      int ok) {
  static tls13_test_trust_s st;
  fio_tls13_server_s server;
  fio_tls13_server_init(&server);
  fio_tls13_server_set_cert_chain(&server, s->chain, s->count);
  fio_tls13_server_set_private_key(&server,
                                   FIO_UBUF_INFO2(s->key, s->key_len),
                                   (fio_tls13_signature_algo_e)s->algo);
  if (cl) {
    fio_tls13_server_require_client_cert(&server, 2);
    fio_tls13_server_set_trust_store(&server,
                                     tls13_test_trust(&st, server_root));
  }

  SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
  FIO_ASSERT(ctx, "SSL_CTX_new failed");
  SSL_CTX_set_min_proto_version(ctx, TLS1_3_VERSION);
  SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
  X509 *ca = tls13_test_ossl_x509(ca_pem);
  FIO_ASSERT(X509_STORE_add_cert(SSL_CTX_get_cert_store(ctx), ca) == 1,
             "X509_STORE_add_cert failed");
  X509 *cl_leaf = NULL, *cl_int = NULL;
  EVP_PKEY *cl_key = NULL;
  if (cl) {
    cl_leaf = tls13_test_ossl_x509(cl[0]);
    cl_int = cl[1] ? tls13_test_ossl_x509(cl[1]) : NULL;
    BIO *kb = BIO_new_mem_buf(cl[2], -1);
    cl_key = kb ? PEM_read_bio_PrivateKey(kb, NULL, NULL, NULL) : NULL;
    BIO_free(kb);
    FIO_ASSERT(cl_key && SSL_CTX_use_certificate(ctx, cl_leaf) == 1 &&
                   (!cl_int || SSL_CTX_add1_chain_cert(ctx, cl_int) == 1) &&
                   SSL_CTX_use_PrivateKey(ctx, cl_key) == 1,
               "OpenSSL client certificate setup failed");
  }
  SSL *ssl = tls13_test_ossl_new(ctx);
  SSL_set_tlsext_host_name(ssl, "localhost");
  FIO_ASSERT(SSL_set1_host(ssl, "localhost") == 1, "SSL_set1_host failed");
  SSL_set_connect_state(ssl);
  int r = 0, failed = 0;
  for (int i = 0; i < 16 && !failed; ++i) {
    r = SSL_do_handshake(ssl);
    if (r != 1 && SSL_get_error(ssl, r) != SSL_ERROR_WANT_READ)
      break;
    failed = tls13_test_ossl_pump(ssl, &server, 1);
    if (r == 1 && fio_tls13_server_is_connected(&server))
      break;
  }
  int connected = !failed && r == 1 && fio_tls13_server_is_connected(&server);
  if (ok) {
    FIO_ASSERT(connected && SSL_get_verify_result(ssl) == X509_V_OK &&
                   SSL_version(ssl) == TLS1_3_VERSION,
               "OpenSSL client and native server (scheme 0x%04x) should "
               "verify each other (verify result %ld)",
               server.signature_algo,
               (long)SSL_get_verify_result(ssl));
    FIO_ASSERT(server.signature_algo == s->algo,
               "native server should sign with its key's scheme (0x%04x)",
               server.signature_algo);
    FIO_ASSERT(!cl || (fio_tls13_server_client_cert_verified(&server) &&
                       server.peer_auth.chain.count == (cl[1] ? 2U : 1U)),
               "native server should verify OpenSSL's client certificate");
  } else {
    FIO_ASSERT(!connected || SSL_get_verify_result(ssl) != X509_V_OK,
               "OpenSSL client / native server: verification must fail");
  }
  ERR_clear_error();
  SSL_free(ssl);
  X509_free(ca);
  X509_free(cl_leaf);
  X509_free(cl_int);
  EVP_PKEY_free(cl_key);
  SSL_CTX_free(ctx);
  fio_tls13_server_destroy(&server);
}

/** Native client (verifying) -> OpenSSL server presenting a compressed chain
 * (P-256 or P-384 leaf + CA). With `trust_ok == 0` the client trusts another
 * CA. */
FIO_SFUNC void tls13_test_ossl_server(const char *leaf_pem,
                                      const char *ca_pem,
                                      const char *key_pem,
                                      const char *trust_pem,
                                      int trust_ok) {
  static tls13_test_trust_s ct;
  fio_x509_trust_store_s *trust = tls13_test_trust(&ct, trust_pem);
  SSL_CTX *ctx = SSL_CTX_new(TLS_server_method());
  FIO_ASSERT(ctx, "SSL_CTX_new failed");
  SSL_CTX_set_min_proto_version(ctx, TLS1_3_VERSION);
  X509 *leaf = tls13_test_ossl_x509(leaf_pem);
  X509 *ca = tls13_test_ossl_x509(ca_pem);
  BIO *kb = BIO_new_mem_buf(key_pem, -1);
  EVP_PKEY *pk = kb ? PEM_read_bio_PrivateKey(kb, NULL, NULL, NULL) : NULL;
  BIO_free(kb);
  FIO_ASSERT(pk && SSL_CTX_use_certificate(ctx, leaf) == 1 &&
                 SSL_CTX_add1_chain_cert(ctx, ca) == 1 &&
                 SSL_CTX_use_PrivateKey(ctx, pk) == 1 &&
                 SSL_CTX_check_private_key(ctx) == 1,
             "OpenSSL server setup with a compressed EC chain failed");
  SSL *ssl = tls13_test_ossl_new(ctx);
  SSL_set_accept_state(ssl);

  fio_tls13_client_s client;
  fio_tls13_client_init(&client, "localhost");
  fio_tls13_client_set_trust_store(&client, trust);
  static uint8_t buf[4096];
  int n = fio_tls13_client_start(&client, buf, sizeof(buf));
  FIO_ASSERT(n > 0 && BIO_write(SSL_get_rbio(ssl), buf, n) == n,
             "native client start failed");
  int r = 0, failed = 0;
  for (int i = 0; i < 16 && !failed; ++i) {
    r = SSL_do_handshake(ssl);
    if (r == 1 && fio_tls13_client_is_connected(&client))
      break; /* (post-handshake tickets are not fed to the client) */
    if (r != 1 && SSL_get_error(ssl, r) != SSL_ERROR_WANT_READ)
      break;
    failed = tls13_test_ossl_pump(ssl, &client, 0);
  }
  if (trust_ok) {
    FIO_ASSERT(!failed && r == 1 && fio_tls13_client_is_connected(&client) &&
                   fio_tls13_client_is_cert_verified(&client),
               "native client should verify OpenSSL's compressed chain "
               "(state %s, alert %d)",
               fio_tls13_client_state_name(&client),
               (int)client.alert_description);
  } else {
    FIO_ASSERT(failed && !fio_tls13_client_is_connected(&client) &&
                   !fio_tls13_client_is_cert_verified(&client),
               "native client trusting another CA must fail");
  }
  ERR_clear_error();
  fio_tls13_client_destroy(&client);
  SSL_free(ssl);
  EVP_PKEY_free(pk);
  X509_free(leaf);
  X509_free(ca);
  SSL_CTX_free(ctx);
}

/** Native client presenting `c` (its first `c_count` certificates, 0: all) ->
 * OpenSSL server (compressed P-256 chain) requesting a client certificate
 * with `SSL_VERIFY_PEER` (plus `SSL_VERIFY_FAIL_IF_NO_PEER_CERT` when
 * `required`), trusting only `root_pem`. `sigalgs` (optional) restricts the
 * CertificateRequest's signature schemes. `expect`: 1 verified client chain,
 * 2 handshake without a client certificate, 0 OpenSSL rejects the client. */
FIO_SFUNC void tls13_test_ossl_mtls(tls13_test_cred_s *c,
                                    size_t c_count,
                                    const char *root_pem,
                                    const char *sigalgs,
                                    int required,
                                    int expect) {
  static tls13_test_trust_s ct;
  if (!c_count)
    c_count = c->count;
  SSL_CTX *ctx = SSL_CTX_new(TLS_server_method());
  FIO_ASSERT(ctx, "SSL_CTX_new failed");
  SSL_CTX_set_min_proto_version(ctx, TLS1_3_VERSION);
  X509 *leaf = tls13_test_ossl_x509(fio___test_ecc_p256_leaf);
  X509 *ca = tls13_test_ossl_x509(fio___test_ecc_p256_ca);
  X509 *root = tls13_test_ossl_x509(root_pem);
  BIO *kb = BIO_new_mem_buf(fio___test_ecc_p256_key, -1);
  EVP_PKEY *pk = kb ? PEM_read_bio_PrivateKey(kb, NULL, NULL, NULL) : NULL;
  BIO_free(kb);
  FIO_ASSERT(pk && SSL_CTX_use_certificate(ctx, leaf) == 1 &&
                 SSL_CTX_add1_chain_cert(ctx, ca) == 1 &&
                 SSL_CTX_use_PrivateKey(ctx, pk) == 1 &&
                 X509_STORE_add_cert(SSL_CTX_get_cert_store(ctx), root) == 1,
             "OpenSSL mTLS server setup failed");
  SSL_CTX_set_verify(ctx,
                     SSL_VERIFY_PEER |
                         (required ? SSL_VERIFY_FAIL_IF_NO_PEER_CERT : 0),
                     NULL);
  if (sigalgs)
    FIO_ASSERT(SSL_CTX_set1_client_sigalgs_list(ctx, sigalgs) == 1,
               "SSL_CTX_set1_client_sigalgs_list(%s) failed",
               sigalgs);
  SSL *ssl = tls13_test_ossl_new(ctx);
  SSL_set_accept_state(ssl);

  fio_tls13_client_s client;
  fio_tls13_client_init(&client, "localhost");
  fio_tls13_client_set_trust_store(&client,
                                   tls13_test_trust(&ct,
                                                    fio___test_ecc_p256_ca));
  fio_tls13_client_set_cert_chain(&client,
                                  c->chain,
                                  c_count,
                                  FIO_UBUF_INFO2(c->key, c->key_len),
                                  (fio_tls13_signature_algo_e)c->algo);
  static uint8_t buf[4096];
  int n = fio_tls13_client_start(&client, buf, sizeof(buf));
  FIO_ASSERT(n > 0 && BIO_write(SSL_get_rbio(ssl), buf, n) == n,
             "native client start failed");
  int r = 0, failed = 0;
  for (int i = 0; i < 16 && !failed; ++i) {
    r = SSL_do_handshake(ssl);
    if (r == 1 && fio_tls13_client_is_connected(&client))
      break;
    if (r != 1 && SSL_get_error(ssl, r) != SSL_ERROR_WANT_READ)
      break;
    failed = tls13_test_ossl_pump(ssl, &client, 0);
  }
  X509 *peer = SSL_get1_peer_certificate(ssl);
  STACK_OF(X509) *vchain = SSL_get0_verified_chain(ssl);
  int verified_len = vchain ? sk_X509_num(vchain) : 0;
  /* the verified chain adds the root unless the client already sent it */
  int expect_len = (int)c_count + 1;
  {
    unsigned char *rd = NULL;
    int rl = i2d_X509(root, &rd);
    if (rl > 0 && (size_t)rl == c->chain[c_count - 1].len &&
        !memcmp(rd, c->chain[c_count - 1].buf, (size_t)rl))
      --expect_len;
    OPENSSL_free(rd);
  }
  switch (expect) {
  case 1:
    FIO_ASSERT(r == 1 && peer && SSL_get_verify_result(ssl) == X509_V_OK &&
                   verified_len == expect_len,
               "OpenSSL server should verify the native client's chain "
               "(scheme 0x%04x, %d verified, sigalgs %s)",
               c->algo,
               verified_len,
               sigalgs ? sigalgs : "default");
    break;
  case 2:
    FIO_ASSERT(r == 1 && !peer && fio_tls13_client_is_connected(&client) &&
                   fio_tls13_client_cert_requested(&client),
               "native client should send no certificate (sigalgs %s)",
               sigalgs ? sigalgs : "default");
    break;
  default:
    FIO_ASSERT(r != 1,
               "OpenSSL server must reject the native client (%d, sigalgs "
               "%s)",
               (int)c_count,
               sigalgs ? sigalgs : "default");
  }
  ERR_clear_error();
  X509_free(peer);
  fio_tls13_client_destroy(&client);
  SSL_free(ssl);
  EVP_PKEY_free(pk);
  X509_free(leaf);
  X509_free(ca);
  X509_free(root);
  SSL_CTX_free(ctx);
}

/** P-384 servers / clients, client chains and RSA client certificates
 * against OpenSSL (both directions). */
FIO_SFUNC void test_tls13_p384_openssl(void) {
  static tls13_test_cred_s pc, pu, rsa, mp;
  tls13_test_cred_load(
      &pc,
      fio___test_ecc_p384_key,
      2,
      (const char *[]){fio___test_ecc_p384_leaf, fio___test_ecc_p384_ca});
  tls13_test_cred_load(
      &pu,
      fio___test_ecc_p384u_key_pkcs8,
      2,
      (const char *[]){fio___test_ecc_p384u_leaf, fio___test_ecc_p384u_ca});
  tls13_test_cred_load(
      &rsa,
      fio___test_mtls_rsa_key,
      2,
      (const char *[]){fio___test_mtls_rsa_leaf, fio___test_mtls_int});
  tls13_test_cred_load(
      &mp,
      fio___test_mtls_p384_key,
      2,
      (const char *[]){fio___test_mtls_p384_leaf, fio___test_mtls_int});
  int log_level = FIO_LOG_LEVEL_GET();
  FIO_LOG_LEVEL_SET(FIO_LOG_LEVEL_FATAL); /* expected handshake errors */
  /* OpenSSL client -> native P-384 server (verified), then mTLS with an
   * OpenSSL P-384 / RSA client chain verified by the native server */
  tls13_test_ossl_client(&pc, fio___test_ecc_p384_ca, NULL, NULL, 1);
  tls13_test_ossl_client(&pu, fio___test_ecc_p384u_ca, NULL, NULL, 1);
  tls13_test_ossl_client(&pc, fio___test_ecc_p384u_ca, NULL, NULL, 0);
  tls13_test_ossl_client(&pu,
                         fio___test_ecc_p384u_ca,
                         (const char *[]){fio___test_ecc_p384_leaf,
                                          NULL,
                                          fio___test_ecc_p384_key},
                         fio___test_ecc_p384_ca,
                         1);
  tls13_test_ossl_client(&pc,
                         fio___test_ecc_p384_ca,
                         (const char *[]){fio___test_mtls_rsa_leaf,
                                          fio___test_mtls_int,
                                          fio___test_mtls_rsa_key},
                         fio___test_mtls_root,
                         1);
  tls13_test_ossl_client(&pc,
                         fio___test_ecc_p384_ca,
                         (const char *[]){fio___test_mtls_p384_leaf,
                                          NULL,
                                          fio___test_mtls_p384_key},
                         fio___test_mtls_root,
                         0); /* no intermediate: the native server rejects */
  /* native client certificate -> OpenSSL server (VERIFY_PEER |
   * FAIL_IF_NO_PEER_CERT, trusting the root only) */
  tls13_test_ossl_mtls(&pc, 0, fio___test_ecc_p384_ca, NULL, 1, 1);
  tls13_test_ossl_mtls(&pu, 0, fio___test_ecc_p384u_ca, NULL, 1, 1);
  tls13_test_ossl_mtls(&mp, 0, fio___test_mtls_root, NULL, 1, 1);
  tls13_test_ossl_mtls(&rsa, 0, fio___test_mtls_root, NULL, 1, 1);
  tls13_test_ossl_mtls(&rsa, 1, fio___test_mtls_root, NULL, 1, 0);
  tls13_test_ossl_mtls(&mp, 1, fio___test_mtls_root, NULL, 1, 0);
  tls13_test_ossl_mtls(&pc, 0, fio___test_ecc_p384u_ca, NULL, 1, 0);
  /* the server's schemes decide: RSA falls back to PSS SHA-384; a key with
   * no accepted scheme sends an empty Certificate */
  tls13_test_ossl_mtls(&rsa,
                       0,
                       fio___test_mtls_root,
                       "rsa_pss_rsae_sha384",
                       1,
                       1);
  tls13_test_ossl_mtls(&mp,
                       0,
                       fio___test_mtls_root,
                       "ecdsa_secp384r1_sha384",
                       1,
                       1);
  tls13_test_ossl_mtls(&mp,
                       0,
                       fio___test_mtls_root,
                       "ecdsa_secp256r1_sha256:rsa_pss_rsae_sha256",
                       1,
                       0);
  tls13_test_ossl_mtls(&mp,
                       0,
                       fio___test_mtls_root,
                       "ecdsa_secp256r1_sha256:rsa_pss_rsae_sha256",
                       0,
                       2);
  tls13_test_ossl_mtls(&rsa,
                       0,
                       fio___test_mtls_root,
                       "ecdsa_secp384r1_sha384:ed25519",
                       0,
                       2);
  FIO_LOG_LEVEL_SET(log_level);
  fprintf(stderr,
          "* TLS 1.3 P-384, client chains and RSA client certificates "
          "interoperate with OpenSSL: OK\n");
}

/** OpenSSL client (verifying the mTLS root) -> 405 server over a socketpair,
 * presenting `chain_count` copies of the big leaf+intermediate pair, so the
 * server's flight outgrows the connection's inline output buffer. */
FIO_SFUNC void tls13_test_ossl_big_io(size_t chain_count) {
  fio_io_functions_s fn = fio_tls13_io_functions();
  static tls13_test_cred_s big;
  tls13_test_big_load(&big);
  const char *pems[16];
  size_t count = 0;
  for (size_t i = 0; i < chain_count; ++i) {
    pems[count++] = fio___test_mtls_big_leaf;
    pems[count++] = fio___test_mtls_big_int;
  }
  char bc[64], bk[64];
  fio___test_tls_pem_join(bc, 64, "io-bgc", pems, count);
  fio___test_tls_pem_file(bk, 64, "io-bgk", fio___test_mtls_big_key);
  fio_io_tls_s *stls = fio_io_tls_new();
  fio_io_tls_cert_add(stls, "localhost", bc, bk, NULL);
  fio___tls13_context_s *sctx =
      (fio___tls13_context_s *)fn.build_context(stls, 0);
  FIO_ASSERT(
      sctx && sctx->cert_chain_count == count &&
          (chain_count < 4 || sctx->flight_size > FIO___TLS13_OUT_BUF_CAP),
      "test setup: the long chain should outgrow the inline output "
      "queue (%zu certificates, flight %zu)",
      sctx ? sctx->cert_chain_count : 0,
      sctx ? sctx->flight_size : 0);
  fio___tls13_connection_s *cs = fio___tls13_connection_new(sctx);
  FIO_ASSERT(cs, "connection allocation failed");
  fio___tls13_server_setup(cs);

  SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
  FIO_ASSERT(ctx, "SSL_CTX_new failed");
  SSL_CTX_set_min_proto_version(ctx, TLS1_3_VERSION);
  SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
  X509 *root = tls13_test_ossl_x509(fio___test_mtls_root);
  FIO_ASSERT(X509_STORE_add_cert(SSL_CTX_get_cert_store(ctx), root) == 1,
             "X509_STORE_add_cert failed");
  SSL *ssl = tls13_test_ossl_new(ctx);
  SSL_set_tlsext_host_name(ssl, "localhost");
  FIO_ASSERT(SSL_set1_host(ssl, "localhost") == 1, "SSL_set1_host failed");
  SSL_set_connect_state(ssl);

  fio_socket_i fds[2];
  FIO_ASSERT(!fio_sock_socketpair(fds), "socketpair failed");
  fio_sock_set_non_block(fds[0]);
  fio_sock_set_non_block(fds[1]);
  int r = 0, done = 0;
  char tmp[64];
  for (size_t round = 0; round < 100000 && !done; ++round) {
    if (!cs->handshake_complete) { /* native server IO read + output flush */
      errno = 0;
      if (fio___tls13_read(fds[0], tmp, sizeof(tmp), cs) < 0 &&
          errno != EWOULDBLOCK && errno != EAGAIN) {
        fprintf(stderr,
                "405 server IO read failed (state %s, alert %d)\n",
                fio_tls13_server_state_name(&cs->state.server),
                (int)cs->state.server.alert_description);
        break;
      }
      fio___tls13_out_flush(fds[0], cs);
    }
    SSL_do_handshake(ssl); /* OpenSSL client */
    uint8_t buf[16384];
    ssize_t n = fio_sock_read(fds[1], buf, sizeof(buf));
    if (n > 0)
      FIO_ASSERT(BIO_write(SSL_get_rbio(ssl), buf, (int)n) == n,
                 "BIO_write failed");
    r = SSL_do_handshake(ssl);
    BIO *wb = SSL_get_wbio(ssl);
    while ((n = BIO_read(wb, buf, (int)sizeof(buf))) > 0) {
      size_t sent = 0;
      while (sent < (size_t)n) { /* blocking peer: always fully sent */
        ssize_t w = fio_sock_write(fds[1], buf + sent, (size_t)n - sent);
        FIO_ASSERT(w > 0, "socket write failed");
        sent += (size_t)w;
      }
    }
    done = cs->handshake_complete && r == 1 && !cs->out_buf_len;
  }
  FIO_ASSERT(done && SSL_get_verify_result(ssl) == X509_V_OK &&
                 SSL_version(ssl) == TLS1_3_VERSION,
             "OpenSSL should verify the 405 server's %zu certificate chain "
             "over a multi-record flight (verify %ld, native state %s)",
             count,
             (long)SSL_get_verify_result(ssl),
             fio_tls13_server_state_name(&cs->state.server));
  FIO_ASSERT(cs->out_buf == cs->out_inline &&
                 cs->out_buf_cap == sizeof(cs->out_inline),
             "a sent flight should release the allocated output queue");
  {
    STACK_OF(X509) *sent = SSL_get_peer_cert_chain(ssl);
    FIO_ASSERT(sent && sk_X509_num(sent) == (int)count,
               "OpenSSL should receive the whole %zu certificate chain",
               count);
  }
  ERR_clear_error();
  fio_sock_close(fds[0]);
  fio_sock_close(fds[1]);
  SSL_free(ssl);
  X509_free(root);
  SSL_CTX_free(ctx);
  fio___tls13_connection_free(cs);
  fio___tls13_free_context_task(sctx, NULL);
  fio_io_tls_free(stls);
  unlink(bc);
  unlink(bk);
}

/** > 16 KiB chains against OpenSSL (both directions, mTLS) and a flight
 * larger than the 405 inline output buffer. */
FIO_SFUNC void test_tls13_big_chain_openssl(void) {
  static tls13_test_cred_s big;
  tls13_test_big_load(&big);
  int log_level = FIO_LOG_LEVEL_GET();
  FIO_LOG_LEVEL_SET(FIO_LOG_LEVEL_FATAL); /* expected failures */
  /* OpenSSL client -> native server (and back, with a big client chain) */
  tls13_test_ossl_client(&big, fio___test_mtls_root, NULL, NULL, 1);
  tls13_test_ossl_client(&big, fio___test_ecc_p256_ca, NULL, NULL, 0);
  tls13_test_ossl_client(&big,
                         fio___test_mtls_root,
                         (const char *[]){fio___test_mtls_big_leaf,
                                          fio___test_mtls_big_int,
                                          fio___test_mtls_big_key},
                         fio___test_mtls_root,
                         1);
  /* native client -> OpenSSL server (leaf + intermediate, root trusted) */
  tls13_test_ossl_server(fio___test_mtls_big_leaf,
                         fio___test_mtls_big_int,
                         fio___test_mtls_big_key,
                         fio___test_mtls_root,
                         1);
  /* native client (big chain) -> OpenSSL server requiring a certificate */
  tls13_test_ossl_mtls(&big, 0, fio___test_mtls_root, NULL, 1, 1);
  tls13_test_ossl_mtls(&big, 1, fio___test_mtls_root, NULL, 1, 0);
  tls13_test_ossl_mtls(&big,
                       0,
                       fio___test_mtls_root,
                       "rsa_pss_rsae_sha384",
                       1,
                       1);
  /* 405 IO server: a flight larger than the inline output queue */
  tls13_test_ossl_big_io(2);
  tls13_test_ossl_big_io(4); /* 8 certificates, ~160 KiB flight */
  FIO_LOG_LEVEL_SET(log_level);
  fprintf(stderr,
          "* TLS 1.3 chains over 16 KiB interoperate with OpenSSL: OK\n");
}

FIO_SFUNC void test_tls13_compressed_openssl(void) {
  int log_level = FIO_LOG_LEVEL_GET();
  FIO_LOG_LEVEL_SET(FIO_LOG_LEVEL_FATAL); /* expected handshake errors */
  static tls13_test_cred_s p256;
  tls13_test_cred_load(
      &p256,
      fio___test_ecc_p256_key,
      2,
      (const char *[]){fio___test_ecc_p256_leaf, fio___test_ecc_p256_ca});
  for (int ok = 1; ok >= 0; --ok) {
    tls13_test_ossl_client(&p256,
                           ok ? fio___test_ecc_p256_ca : fio___test_ecc_p384_ca,
                           NULL,
                           NULL,
                           ok);
    tls13_test_ossl_server(fio___test_ecc_p256_leaf,
                           fio___test_ecc_p256_ca,
                           fio___test_ecc_p256_key,
                           ok ? fio___test_ecc_p256_ca
                              : fio___test_ecc_p384_ca,
                           ok);
    tls13_test_ossl_server(fio___test_ecc_p384_leaf,
                           fio___test_ecc_p384_ca,
                           fio___test_ecc_p384_key,
                           ok ? fio___test_ecc_p384_ca
                              : fio___test_ecc_p256_ca,
                           ok);
  }
  FIO_LOG_LEVEL_SET(log_level);
  fprintf(stderr,
          "* TLS 1.3 compressed EC chains interoperate with OpenSSL: OK\n");
}
#else
FIO_SFUNC void test_tls13_compressed_openssl(void) {}
FIO_SFUNC void test_tls13_p384_openssl(void) {}
FIO_SFUNC void test_tls13_big_chain_openssl(void) {}
#endif /* HAVE_OPENSSL && FIO_OS_POSIX */

/* *****************************************************************************
Server certificate selection by SNI (several certificates, one context)
***************************************************************************** */

/** Handshakes a client (SNI `sni`) with a server connection of `sctx` and
 * returns the CN of the certificate the server presented. */
FIO_SFUNC fio_buf_info_s tls13_test_sni_handshake(fio___tls13_context_s *sctx,
                                                  const char *sni,
                                                  char *cn_buf) {
  fio___tls13_context_s cctx = {.is_client = 1, .ref = 1};
  fio___tls13_connection_s *conn_s = fio___tls13_connection_new(sctx);
  fio___tls13_connection_s *conn_c = fio___tls13_connection_new(&cctx);
  FIO_ASSERT(conn_s && conn_c, "connection allocation failed");
  fio___tls13_server_setup(conn_s);
  fio_tls13_client_init(&conn_c->state.client, sni);
  fio_tls13_client_skip_verification(&conn_c->state.client, 1);
  FIO_ASSERT(tls13_test_run_handshake(&conn_c->state.client,
                                      &conn_s->state.server,
                                      0,
                                      NULL,
                                      NULL,
                                      0),
             "SNI handshake failed (%s)",
             sni ? sni : "no SNI");
  conn_c->handshake_complete = 1;
  fio_x509_cert_s info = {0};
  FIO_ASSERT(!fio___tls13_peer_info_next(-1, &info, conn_c) && info.cn.len < 64,
             "peer certificate missing (%s)",
             sni ? sni : "no SNI");
  FIO_MEMCPY(cn_buf, info.cn.buf, info.cn.len);
  cn_buf[info.cn.len] = 0;
  fio___tls13_connection_free(conn_c);
  fio___tls13_connection_free(conn_s);
  return FIO_BUF_INFO2(cn_buf, info.cn.len);
}

FIO_SFUNC void test_tls13_sni_cert_selection(void) {
  fio_io_functions_s fn = fio_tls13_io_functions();
  struct {
    const char *sni;
    const char *cn; /* expected certificate (self-signed CN) */
  } cases[] = {
      {"a.example.com", "a.example.com"},
      {"B.EXAMPLE.com", "b.example.com"}, /* case-insensitive */
      {"x.w.example.com", "*.w.example.com"},
      {"w.example.com", "localhost"},     /* wildcard covers one label only */
      {"a.x.w.example.com", "localhost"}, /* ... exactly one */
      {"unknown.example.org", "localhost"},
      {NULL, "localhost"},
  };
  { /* explicit default (empty name), added between named certificates */
    fio_io_tls_s *tls = fio_io_tls_new();
    fio_io_tls_cert_add(tls, "a.example.com", NULL, NULL, NULL);
    fio_io_tls_cert_add(tls, NULL, NULL, NULL, NULL); /* "localhost" */
    fio_io_tls_cert_add(tls, "B.Example.COM", NULL, NULL, NULL);
    fio_io_tls_cert_add(tls, "*.w.example.com", NULL, NULL, NULL);
    FIO_ASSERT(fio_io_tls_cert_count(tls) == 4, "certificate count error");
    fio___tls13_context_s *ctx =
        (fio___tls13_context_s *)fn.build_context(tls, 0);
    FIO_ASSERT(ctx && ctx->creds_count == 4 && ctx->sni.dflt == 1,
               "SNI context build failed");
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
      char cn[64];
      fio_buf_info_s got = tls13_test_sni_handshake(ctx, cases[i].sni, cn);
      FIO_ASSERT(FIO_BUF_INFO_IS_EQ(got, FIO_BUF_INFO1((char *)cases[i].cn)),
                 "SNI %s should select %s, got %s",
                 cases[i].sni ? cases[i].sni : "(none)",
                 cases[i].cn,
                 cn);
    }
    /* malformed / oversized names select the default (shared matcher) */
    char big[300];
    FIO_MEMSET(big, 'a', sizeof(big));
    FIO_ASSERT(fio___io_tls_sni_find(&ctx->sni, "a.example.com", 13) == 0,
               "exact SNI lookup failed");
    struct {
      const char *nm;
      size_t len;
    } bad[] = {
        {big, 256},
        {"a.example.com\0", 14},
        {"a example.com", 13},
        {"*.w.example.com", 15},
        {".w.example.com", 14},
        {"", 0},
    };
    for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); ++i)
      FIO_ASSERT(fio___io_tls_sni_find(&ctx->sni, bad[i].nm, bad[i].len) == 1,
                 "malformed SNI names must select the default certificate");
    fio___tls13_free_context_task(ctx, NULL);
    fio_io_tls_free(tls);
  }
  { /* no empty-name certificate: the first one added is the default */
    fio_io_tls_s *tls = fio_io_tls_new();
    fio_io_tls_cert_add(tls, "first.example.com", NULL, NULL, NULL);
    fio_io_tls_cert_add(tls, "second.example.com", NULL, NULL, NULL);
    fio___tls13_context_s *ctx =
        (fio___tls13_context_s *)fn.build_context(tls, 0);
    FIO_ASSERT(ctx && ctx->sni.dflt == 0, "SNI context build failed");
    char cn[64];
    FIO_ASSERT(
        FIO_BUF_INFO_IS_EQ(
            tls13_test_sni_handshake(ctx, "second.example.com", cn),
            FIO_BUF_INFO1((char *)"second.example.com")) &&
            FIO_BUF_INFO_IS_EQ(
                tls13_test_sni_handshake(ctx, "other.example.com", cn),
                FIO_BUF_INFO1((char *)"first.example.com")) &&
            FIO_BUF_INFO_IS_EQ(tls13_test_sni_handshake(ctx, NULL, cn),
                               FIO_BUF_INFO1((char *)"first.example.com")),
        "first certificate should be the default");
    fio___tls13_free_context_task(ctx, NULL);
    fio_io_tls_free(tls);
  }
  fprintf(stderr, "* TLS 1.3 SNI certificate selection: OK\n");
}

/* *****************************************************************************
Main
***************************************************************************** */
int main(void) {
  test_tls13_key_schedule();
  test_tls13_handshake_messages();
  test_tls13_client_hello_limits();
  test_tls13_hello_retry_rules();
  test_tls13_certificate_list_strict();
  test_tls13_key_update();
  test_tls13_handshake_roundtrip();
  test_tls13_handshake_fragmentation();
  test_tls13_handshake_alpn();
  test_tls13_handshake_client_cert();
  test_tls13_handshake_client_cert_trusted();
  test_tls13_handshake_client_cert_untrusted();
  test_tls13_client_verifies_server();
  test_tls13_app_data();
  test_tls13_large_transfer();
  test_tls13_write_key_update_pending();
  test_tls13_finish_close_notify_order();
  test_tls13_read_hardening();
  test_tls13_alpn_dispatch();
  test_tls13_context_overhead();
  test_tls13_io_functions();
  test_tls13_sni_from_tls_object();
  test_tls13_sni_cert_selection();
  test_tls13_server_pem_p256_derived_pubkey();
  test_tls13_compressed_key_match();
  test_tls13_compressed_handshake();
  test_tls13_compressed_openssl();
  test_tls13_p384_key_match();
  test_tls13_p384_handshake();
  test_tls13_client_cert_chain();
  test_tls13_sig_select();
  test_tls13_io_client_cert();
  test_tls13_rsa_key_crt();
  test_tls13_big_chain();
  test_tls13_io_big_chain();
  test_tls13_io_out_reserve();
  test_tls13_p384_openssl();
  test_tls13_big_chain_openssl();
  /* encrypted private keys are unsupported: any password fails closed */
  fio___test_tls_pem_load(fio_tls13_io_functions(), "TLS 1.3", 0);
  fio___test_tls_must(fio_tls13_io_functions(), "TLS 1.3");
  test_tls13_peer_info_next();
  fio___test_tls_connect_verify(fio_tls13_io_functions(), "TLS 1.3");
  return 0;
}
