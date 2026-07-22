/*****************************************************************************
Test: TLS 1.3 roundtrips with in-process fio TLS 1.3 servers and an OpenSSL
libssl client.

Replaces the external-process oracle tests tests-old/tls13-curl.c and
tests-old/tls13-openssl.c with deterministic, library-only roundtrips.  The
facil.io side uses the native TLS 1.3 IO implementation
(fio_tls13_io_functions); the client uses OpenSSL's libssl directly from the
same process.  In addition to HTTP interoperability, this test verifies that
an OpenSSL client certificate is exposed through fio_io_peer_info_next and that
partially consumed plaintext schedules enough deferred on_data callbacks to
drain the TLS buffer when the peer sends no more application data.

Without OpenSSL the test compiles to a no-op that exits 0.
***************************************************************************** */
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
#define FIO_HTTP
#if HAVE_OPENSSL
#define FIO_OPENSSL
#endif
#include "../tests/test-helpers.h"

#if HAVE_OPENSSL && FIO_OS_POSIX

#include <arpa/inet.h>
#include <openssl/err.h>
#include <openssl/obj_mac.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

/* *****************************************************************************
Shared state
***************************************************************************** */

enum {
  FIO___TLS13_OPENSSL_RT_PARTIAL_READ = 17,
  FIO___TLS13_OPENSSL_RT_PHASES = 2,
};

static const size_t fio___tls13_openssl_rt_payload_len[] = {
    4096,
    FIO_TLS13_MAX_PLAINTEXT_LEN + 257,
};
static const char fio___tls13_openssl_rt_client_cn[] = "openssl-client";

typedef struct {
  fio_http_listener_s *http_listener;
  fio_io_listener_s *partial_listener;
  fio_io_listener_s *openssl_peer_listener;
  X509 *client_cert;
  EVP_PKEY *client_key;
  int http_port;
  int partial_port;
  int openssl_peer_port;
  int result;
  int done;
  size_t partial_phase;
  size_t partial_received[FIO___TLS13_OPENSSL_RT_PHASES];
  size_t partial_on_data[FIO___TLS13_OPENSSL_RT_PHASES];
  int partial_peer_cert_seen;
  int openssl_peer_cert_seen;
  int partial_failed;
  int openssl_peer_failed;
  int thread_started;
  pthread_t thread;
} fio___tls13_openssl_rt_state_s;

static fio___tls13_openssl_rt_state_s fio___tls13_openssl_rt_state = {0};
static fio_io_protocol_s fio___tls13_openssl_rt_partial_protocol = {0};
static fio_io_protocol_s fio___tls13_openssl_rt_openssl_peer_protocol = {0};
static int fio___tls13_openssl_rt_watchdog_fired = 0;

/* *****************************************************************************
HTTP server handler
***************************************************************************** */

static void fio___tls13_openssl_rt_on_http(fio_http_s *h) {
  fio_http_status_set(h, 200);
  fio_http_write(h, .buf = "OK", .len = 2, .finish = 1);
}

static uint8_t fio___tls13_openssl_rt_payload_byte(size_t phase, size_t pos) {
  return (uint8_t)((pos * 131U + phase * 17U + 29U) & 255U);
}

static int fio___tls13_openssl_rt_peer_cert_validate(fio_io_s *io,
                                                      const char *backend) {
  fio_x509_cert_s cert = {0};
  int peer_info_result = fio_io_peer_info_next(io, &cert);
  if (peer_info_result || !cert.der.buf || !cert.der.len || cert.chain_index ||
      !cert.verified || !cert.cn.buf ||
      cert.cn.len != sizeof(fio___tls13_openssl_rt_client_cn) - 1 ||
      memcmp(cert.cn.buf, fio___tls13_openssl_rt_client_cn, cert.cn.len)) {
    fprintf(stderr,
            "%s fio_io_peer_info_next client certificate mismatch: "
            "result=%d, DER=%zu, chain_index=%u, verified=%u, CN=%.*s\n",
            backend,
            peer_info_result,
            cert.der.len,
            (unsigned)cert.chain_index,
            (unsigned)cert.verified,
            (int)cert.cn.len,
            cert.cn.buf ? cert.cn.buf : "");
    return -1;
  }
  if (fio_io_peer_info_next(io, &cert) != -1) {
    fprintf(stderr,
            "%s fio_io_peer_info_next returned an unexpected client chain "
            "certificate\n",
            backend);
    return -1;
  }
  return 0;
}

static void fio___tls13_openssl_rt_on_data_partial(fio_io_s *io) {
  fio___tls13_openssl_rt_state_s *state =
      (fio___tls13_openssl_rt_state_s *)fio_io_udata(io);
  const size_t phase = state->partial_phase;
  uint8_t buf[FIO___TLS13_OPENSSL_RT_PARTIAL_READ];

  if (phase >= FIO___TLS13_OPENSSL_RT_PHASES) {
    if (fio_io_read(io, buf, sizeof(buf))) {
      state->partial_failed = 1;
      fio_io_close(io);
    }
    return;
  }

  /* Deliberately perform only one small read per callback.  Once OpenSSL has
   * sent the phase payload it waits for our acknowledgement, so retained TLS
   * plaintext must schedule the remaining callbacks without another peer
   * write or socket readability edge. */
  size_t len = fio_io_read(io, buf, sizeof(buf));
  if (!len)
    return;

  if (!state->partial_peer_cert_seen) {
    if (fio___tls13_openssl_rt_peer_cert_validate(io, "embedded TLS")) {
      state->partial_failed = 1;
      fio_io_close(io);
      return;
    }
    state->partial_peer_cert_seen = 1;
  }

  ++state->partial_on_data[phase];
  const size_t offset = state->partial_received[phase];
  const size_t expected_len = fio___tls13_openssl_rt_payload_len[phase];
  if (len > expected_len - offset) {
    fprintf(stderr,
            "partial-read phase %zu overflow: %zu + %zu > %zu\n",
            phase,
            offset,
            len,
            expected_len);
    state->partial_failed = 1;
    fio_io_close(io);
    return;
  }
  for (size_t i = 0; i < len; ++i) {
    const uint8_t expected =
        fio___tls13_openssl_rt_payload_byte(phase, offset + i);
    if (buf[i] != expected) {
      fprintf(stderr,
              "partial-read phase %zu mismatch at %zu: %u != %u\n",
              phase,
              offset + i,
              (unsigned)buf[i],
              (unsigned)expected);
      state->partial_failed = 1;
      fio_io_close(io);
      return;
    }
  }

  state->partial_received[phase] += len;
  if (state->partial_received[phase] == expected_len) {
    uint8_t ack = (uint8_t)('A' + phase);
    ++state->partial_phase;
    fio_io_write(io, &ack, 1);
  }
}

static void fio___tls13_openssl_rt_on_data_openssl_peer(fio_io_s *io) {
  fio___tls13_openssl_rt_state_s *state =
      (fio___tls13_openssl_rt_state_s *)fio_io_udata(io);
  uint8_t buf[2];
  size_t len = fio_io_read(io, buf, sizeof(buf));
  if (!len)
    return;
  if (state->openssl_peer_cert_seen || len != 1 || buf[0] != 'P' ||
      fio___tls13_openssl_rt_peer_cert_validate(io, "OpenSSL")) {
    state->openssl_peer_failed = 1;
    fio_io_close(io);
    return;
  }
  state->openssl_peer_cert_seen = 1;
  fio_io_write(io, buf, 1);
}

/* *****************************************************************************
OpenSSL libssl client
***************************************************************************** */

typedef struct {
  void *protocol;
  void *udata;
  void *tls_ctx;
  void *queue_for_accept;
  void *queue;
  void *io;
  void (*on_start)(void *protocol, void *udata);
  void (*on_stop)(void *protocol, void *udata);
  int owner;
  int fd;
  size_t ref_count;
  size_t url_len;
  uint8_t hide_from_log;
  char url[];
} fio___tls13_openssl_rt_listen_s;

static int fio___tls13_openssl_rt_listener_port(fio_io_listener_s *listener) {
  fio___tls13_openssl_rt_listen_s *l =
      (fio___tls13_openssl_rt_listen_s *)listener;
  struct sockaddr_in addr = {0};
  socklen_t len = sizeof(addr);
  if (getsockname(l->fd, (struct sockaddr *)&addr, &len) != 0 ||
      addr.sin_family != AF_INET)
    return -1;
  return ntohs(addr.sin_port);
}

static int fio___tls13_openssl_rt_socket_connect(const char *host, int port) {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  FIO_ASSERT(fd >= 0, "client socket failed");

  struct timeval tv = {.tv_sec = 5, .tv_usec = 0};
  setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
  setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_port = htons((uint16_t)port);
  if (inet_pton(AF_INET, host, &addr.sin_addr) != 1)
    FIO_ASSERT(0, "inet_pton failed");

  if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    fprintf(stderr, "client connect failed: %s\n", strerror(errno));
    close(fd);
    return -1;
  }
  return fd;
}

static int fio___tls13_openssl_rt_client_identity_create(char *trust_path) {
  fio___tls13_openssl_rt_state_s *state = &fio___tls13_openssl_rt_state;
  EVP_PKEY_CTX *kctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, NULL);
  EVP_PKEY *key = NULL;
  X509 *cert = NULL;
  FILE *file = NULL;
  int fd = -1;
  int trust_file_created = 0;
  int result = -1;

  if (!kctx || EVP_PKEY_keygen_init(kctx) <= 0 ||
      EVP_PKEY_CTX_set_ec_paramgen_curve_nid(kctx,
                                             NID_X9_62_prime256v1) <= 0 ||
      EVP_PKEY_keygen(kctx, &key) <= 0 || !(cert = X509_new()) ||
      !X509_set_version(cert, 2) ||
      !ASN1_INTEGER_set(X509_get_serialNumber(cert), 1) ||
      !X509_gmtime_adj(X509_getm_notBefore(cert), -3600) ||
      !X509_gmtime_adj(X509_getm_notAfter(cert), 3600) ||
      !X509_NAME_add_entry_by_txt(
          X509_get_subject_name(cert),
          "CN",
          MBSTRING_ASC,
          (const unsigned char *)fio___tls13_openssl_rt_client_cn,
          -1,
          -1,
          0) ||
      !X509_set_issuer_name(cert, X509_get_subject_name(cert)) ||
      !X509_set_pubkey(cert, key) || X509_sign(cert, key, EVP_sha256()) <= 0)
    goto cleanup;

  fd = mkstemp(trust_path);
  if (fd < 0)
    goto cleanup;
  trust_file_created = 1;
  file = fdopen(fd, "w");
  if (!file)
    goto cleanup;

  int write_ok = PEM_write_X509(file, cert) == 1;
  if (fclose(file))
    write_ok = 0;
  file = NULL;
  fd = -1;
  if (!write_ok)
    goto cleanup;
  state->client_cert = cert;
  state->client_key = key;
  cert = NULL;
  key = NULL;
  result = 0;

cleanup:
  if (file)
    fclose(file);
  else if (fd >= 0)
    close(fd);
  EVP_PKEY_CTX_free(kctx);
  X509_free(cert);
  EVP_PKEY_free(key);
  if (result) {
    if (trust_file_created)
      unlink(trust_path);
    ERR_print_errors_fp(stderr);
  }
  return result;
}

static int fio___tls13_openssl_rt_client_cert_add(SSL_CTX *ctx) {
  fio___tls13_openssl_rt_state_s *state = &fio___tls13_openssl_rt_state;
  if (SSL_CTX_use_certificate(ctx, state->client_cert) != 1 ||
      SSL_CTX_use_PrivateKey(ctx, state->client_key) != 1 ||
      SSL_CTX_check_private_key(ctx) != 1) {
    fprintf(stderr, "failed to load the OpenSSL client certificate\n");
    return -1;
  }
  return 0;
}

static int fio___tls13_openssl_rt_client_run(const char *host, int port) {
  SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
  if (!ctx)
    return -1;
  if (fio___tls13_openssl_rt_client_cert_add(ctx)) {
    SSL_CTX_free(ctx);
    return -1;
  }

  SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);
  SSL_CTX_set_min_proto_version(ctx, TLS1_3_VERSION);
  SSL_CTX_set_max_proto_version(ctx, TLS1_3_VERSION);

  static const unsigned char alpn[] = "\x8http/1.1";
  SSL_CTX_set_alpn_protos(ctx, alpn, sizeof(alpn) - 1);

  int fd = fio___tls13_openssl_rt_socket_connect(host, port);
  if (fd < 0) {
    SSL_CTX_free(ctx);
    return -1;
  }

  SSL *ssl = SSL_new(ctx);
  if (!ssl) {
    close(fd);
    SSL_CTX_free(ctx);
    return -1;
  }
  SSL_set_fd(ssl, fd);

  if (SSL_connect(ssl) <= 0) {
    int err = SSL_get_error(ssl, -1);
    fprintf(stderr, "SSL_connect failed: err=%d\n", err);
    SSL_free(ssl);
    close(fd);
    SSL_CTX_free(ctx);
    return -1;
  }

  const char *version = SSL_get_version(ssl);
  if (!version || strcmp(version, "TLSv1.3") != 0) {
    fprintf(stderr,
            "TLS version mismatch: expected TLSv1.3, got %s\n",
            version ? version : "(null)");
    SSL_free(ssl);
    close(fd);
    SSL_CTX_free(ctx);
    return -1;
  }

  const unsigned char *negotiated = NULL;
  unsigned int negotiated_len = 0;
  SSL_get0_alpn_selected(ssl, &negotiated, &negotiated_len);
  if (negotiated_len != 8 || FIO_MEMCMP(negotiated, "http/1.1", 8) != 0) {
    fprintf(stderr, "ALPN mismatch: expected http/1.1\n");
    SSL_free(ssl);
    close(fd);
    SSL_CTX_free(ctx);
    return -1;
  }

  static const char request[] = "GET / HTTP/1.1\r\n"
                                "Host: localhost\r\n"
                                "Connection: close\r\n"
                                "\r\n";
  if (SSL_write(ssl, request, sizeof(request) - 1) <= 0) {
    SSL_free(ssl);
    close(fd);
    SSL_CTX_free(ctx);
    return -1;
  }

  char response[4096];
  int total = 0;
  for (;;) {
    int n = SSL_read(ssl, response + total, (int)sizeof(response) - total - 1);
    if (n > 0) {
      total += n;
      continue;
    }
    break;
  }
  response[total] = 0;

  int ok = 1;
  if (total <= 0) {
    fprintf(stderr, "client received empty response\n");
    ok = 0;
  } else if (!strstr(response, "HTTP/1.1 200 OK")) {
    fprintf(stderr, "response status mismatch: %.*s\n", total, response);
    ok = 0;
  } else if (!strstr(response, "OK")) {
    fprintf(stderr, "response body mismatch: %.*s\n", total, response);
    ok = 0;
  }

  SSL_shutdown(ssl);
  SSL_free(ssl);
  close(fd);
  SSL_CTX_free(ctx);
  return ok ? 0 : -1;
}

static int fio___tls13_openssl_rt_partial_client_run(const char *host,
                                                       int port) {
  SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
  if (!ctx)
    return -1;
  if (fio___tls13_openssl_rt_client_cert_add(ctx)) {
    SSL_CTX_free(ctx);
    return -1;
  }

  SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);
  SSL_CTX_set_min_proto_version(ctx, TLS1_3_VERSION);
  SSL_CTX_set_max_proto_version(ctx, TLS1_3_VERSION);

  int fd = fio___tls13_openssl_rt_socket_connect(host, port);
  if (fd < 0) {
    SSL_CTX_free(ctx);
    return -1;
  }

  SSL *ssl = SSL_new(ctx);
  if (!ssl) {
    close(fd);
    SSL_CTX_free(ctx);
    return -1;
  }
  SSL_set_fd(ssl, fd);

  int ok = 1;
  if (SSL_connect(ssl) <= 0) {
    fprintf(stderr, "partial-read SSL_connect failed\n");
    ok = 0;
  }

  uint8_t payload[FIO_TLS13_MAX_PLAINTEXT_LEN + 257];
  for (size_t phase = 0; ok && phase < FIO___TLS13_OPENSSL_RT_PHASES;
       ++phase) {
    const size_t len = fio___tls13_openssl_rt_payload_len[phase];
    for (size_t i = 0; i < len; ++i)
      payload[i] = fio___tls13_openssl_rt_payload_byte(phase, i);

    size_t sent = 0;
    while (sent < len) {
      int written = SSL_write(ssl, payload + sent, (int)(len - sent));
      if (written <= 0) {
        fprintf(stderr, "partial-read SSL_write failed in phase %zu\n", phase);
        ok = 0;
        break;
      }
      sent += (size_t)written;
    }
    if (!ok)
      break;

    /* Do not send more application data until the server drains this phase.
     * A missing deferred on_data event therefore deadlocks here and is caught
     * by the socket timeout / reactor watchdog. */
    uint8_t ack = 0;
    int ack_len = SSL_read(ssl, &ack, 1);
    if (ack_len != 1 || ack != (uint8_t)('A' + phase)) {
      fprintf(stderr,
              "partial-read acknowledgement failed in phase %zu\n",
              phase);
      ok = 0;
    }
  }

  SSL_shutdown(ssl);
  SSL_free(ssl);
  close(fd);
  SSL_CTX_free(ctx);
  return ok ? 0 : -1;
}

static int fio___tls13_openssl_rt_peer_client_run(const char *host, int port) {
  SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
  if (!ctx)
    return -1;
  if (fio___tls13_openssl_rt_client_cert_add(ctx)) {
    SSL_CTX_free(ctx);
    return -1;
  }
  SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);
  SSL_CTX_set_min_proto_version(ctx, TLS1_3_VERSION);
  SSL_CTX_set_max_proto_version(ctx, TLS1_3_VERSION);

  int fd = fio___tls13_openssl_rt_socket_connect(host, port);
  if (fd < 0) {
    SSL_CTX_free(ctx);
    return -1;
  }
  SSL *ssl = SSL_new(ctx);
  if (!ssl) {
    close(fd);
    SSL_CTX_free(ctx);
    return -1;
  }
  SSL_set_fd(ssl, fd);

  uint8_t byte = 'P';
  int ok = 1;
  if (SSL_connect(ssl) <= 0) {
    fprintf(stderr, "OpenSSL-backend SSL_connect failed\n");
    ok = 0;
  } else if (SSL_write(ssl, &byte, 1) != 1) {
    fprintf(stderr, "OpenSSL-backend SSL_write failed\n");
    ok = 0;
  } else {
    byte = 0;
    if (SSL_read(ssl, &byte, 1) != 1 || byte != 'P') {
      fprintf(stderr, "OpenSSL-backend acknowledgement failed\n");
      ok = 0;
    }
  }

  SSL_shutdown(ssl);
  SSL_free(ssl);
  close(fd);
  SSL_CTX_free(ctx);
  return ok ? 0 : -1;
}

static void *fio___tls13_openssl_rt_thread(void *ignr_) {
  (void)ignr_;
  /* Give both listeners a moment to start accepting. */
  usleep(100000);

  fio___tls13_openssl_rt_state.http_port =
      fio___tls13_openssl_rt_listener_port((fio_io_listener_s *)
                                               fio___tls13_openssl_rt_state
                                                   .http_listener);
  fio___tls13_openssl_rt_state.partial_port =
      fio___tls13_openssl_rt_listener_port(
          fio___tls13_openssl_rt_state.partial_listener);
  fio___tls13_openssl_rt_state.openssl_peer_port =
      fio___tls13_openssl_rt_listener_port(
          fio___tls13_openssl_rt_state.openssl_peer_listener);
  if (fio___tls13_openssl_rt_state.http_port <= 0 ||
      fio___tls13_openssl_rt_state.partial_port <= 0 ||
      fio___tls13_openssl_rt_state.openssl_peer_port <= 0) {
    fio___tls13_openssl_rt_state.result = -1;
  } else if (fio___tls13_openssl_rt_client_run(
                 "127.0.0.1", fio___tls13_openssl_rt_state.http_port) ||
             fio___tls13_openssl_rt_partial_client_run(
                 "127.0.0.1", fio___tls13_openssl_rt_state.partial_port) ||
             fio___tls13_openssl_rt_peer_client_run(
                 "127.0.0.1",
                 fio___tls13_openssl_rt_state.openssl_peer_port)) {
    fio___tls13_openssl_rt_state.result = -1;
  }
  fio_atomic_exchange(&fio___tls13_openssl_rt_state.done, 1);
  fio_io_stop();
  return NULL;
}

/* *****************************************************************************
Reactor callbacks
***************************************************************************** */

static void fio___tls13_openssl_rt_start_client(void *ignr_) {
  (void)ignr_;
  if (pthread_create(&fio___tls13_openssl_rt_state.thread,
                     NULL,
                     fio___tls13_openssl_rt_thread,
                     NULL) != 0) {
    fio___tls13_openssl_rt_state.result = -1;
    fio_atomic_exchange(&fio___tls13_openssl_rt_state.done, 1);
    fio_io_stop();
  } else {
    fio___tls13_openssl_rt_state.thread_started = 1;
  }
}

static int fio___tls13_openssl_rt_watchdog(void *u1, void *u2) {
  (void)u1;
  (void)u2;
  int done;
  fio_atomic_load(done, &fio___tls13_openssl_rt_state.done);
  if (!done) {
    fio___tls13_openssl_rt_watchdog_fired = 1;
    fio_io_stop();
  }
  return -1;
}

/* *****************************************************************************
Main
***************************************************************************** */

int main(void) {
  fprintf(stderr,
          "Testing TLS 1.3 roundtrips (embedded TLS + OpenSSL backends):\n");

  fio_io_functions_s tls13_funcs = fio_tls13_io_functions();
  fio_io_functions_s openssl_funcs = fio_openssl_io_functions();
  fio___tls13_openssl_rt_partial_protocol = (fio_io_protocol_s){
      .on_data = fio___tls13_openssl_rt_on_data_partial,
      .on_timeout = fio_io_touch,
      .io_functions = tls13_funcs,
      .timeout = 5000,
  };
  fio___tls13_openssl_rt_openssl_peer_protocol = (fio_io_protocol_s){
      .on_data = fio___tls13_openssl_rt_on_data_openssl_peer,
      .on_timeout = fio_io_touch,
      .io_functions = openssl_funcs,
      .timeout = 5000,
  };

  char trust_path[] = ".tls13-openssl-client-cert-XXXXXX";
  FIO_ASSERT(!fio___tls13_openssl_rt_client_identity_create(trust_path),
             "failed to create the OpenSSL client identity");

  fio_io_tls_s *tls = fio_io_tls_new();
  FIO_ASSERT(tls, "fio_io_tls_new failed");
  fio_io_tls_cert_add(tls, "localhost", NULL, NULL, NULL);
  FIO_ASSERT(fio_io_tls_trust_add(tls, trust_path),
             "fio_io_tls_trust_add failed");
  fio_io_tls_alpn_add(tls, "http/1.1", NULL);

  fio___tls13_openssl_rt_state.http_listener =
      fio_http_listen("https://127.0.0.1:0",
                      .tls = tls,
                      .tls_io_func = &tls13_funcs,
                      .on_http = fio___tls13_openssl_rt_on_http,
                      .timeout = 5);
  fio___tls13_openssl_rt_state.partial_listener =
      fio_io_listen(.url = "tcp://127.0.0.1:0",
                    .protocol = &fio___tls13_openssl_rt_partial_protocol,
                    .udata = &fio___tls13_openssl_rt_state,
                    .tls = tls,
                    .hide_from_log = 1);
  fio___tls13_openssl_rt_state.openssl_peer_listener =
      fio_io_listen(.url = "tcp://127.0.0.1:0",
                    .protocol = &fio___tls13_openssl_rt_openssl_peer_protocol,
                    .udata = &fio___tls13_openssl_rt_state,
                    .tls = tls,
                    .hide_from_log = 1);
  fio_io_tls_free(tls);
  FIO_ASSERT(!unlink(trust_path), "failed to remove temporary trust file");
  FIO_ASSERT(fio___tls13_openssl_rt_state.http_listener,
             "fio_http_listen failed on 127.0.0.1:0");
  FIO_ASSERT(fio___tls13_openssl_rt_state.partial_listener,
             "embedded TLS fio_io_listen failed on 127.0.0.1:0");
  FIO_ASSERT(fio___tls13_openssl_rt_state.openssl_peer_listener,
             "OpenSSL fio_io_listen failed on 127.0.0.1:0");

  fio_state_callback_add(FIO_CALL_ON_START,
                         fio___tls13_openssl_rt_start_client,
                         NULL);
  fio_io_run_every(.fn = fio___tls13_openssl_rt_watchdog,
                   .every = 8000,
                   .repetitions = 1);

  fio_io_start(0);

  fio_state_callback_remove(FIO_CALL_ON_START,
                            fio___tls13_openssl_rt_start_client,
                            NULL);
  if (fio___tls13_openssl_rt_state.thread_started)
    pthread_join(fio___tls13_openssl_rt_state.thread, NULL);

  fio_io_listen_stop(
      (fio_io_listener_s *)fio___tls13_openssl_rt_state.http_listener);
  fio_io_listen_stop(fio___tls13_openssl_rt_state.partial_listener);
  fio_io_listen_stop(fio___tls13_openssl_rt_state.openssl_peer_listener);
  X509_free(fio___tls13_openssl_rt_state.client_cert);
  EVP_PKEY_free(fio___tls13_openssl_rt_state.client_key);

  FIO_ASSERT(!fio___tls13_openssl_rt_watchdog_fired,
             "TLS 1.3 roundtrip timed out");
  FIO_ASSERT(fio___tls13_openssl_rt_state.done,
             "client thread did not complete");
  FIO_ASSERT(fio___tls13_openssl_rt_state.result == 0,
             "TLS 1.3 OpenSSL client roundtrip failed");
  FIO_ASSERT(!fio___tls13_openssl_rt_state.partial_failed,
             "TLS 1.3 partial-read data validation failed");
  FIO_ASSERT(fio___tls13_openssl_rt_state.partial_peer_cert_seen,
             "embedded TLS fio_io_peer_info_next did not expose the OpenSSL "
             "client certificate");
  FIO_ASSERT(!fio___tls13_openssl_rt_state.openssl_peer_failed,
             "OpenSSL peer certificate validation failed");
  FIO_ASSERT(fio___tls13_openssl_rt_state.openssl_peer_cert_seen,
             "OpenSSL fio_io_peer_info_next did not expose the client leaf "
             "certificate");
  FIO_ASSERT(fio___tls13_openssl_rt_state.partial_phase ==
                 FIO___TLS13_OPENSSL_RT_PHASES,
             "TLS 1.3 partial-read phases did not complete");
  for (size_t phase = 0; phase < FIO___TLS13_OPENSSL_RT_PHASES; ++phase) {
    const size_t expected_len = fio___tls13_openssl_rt_payload_len[phase];
    const size_t expected_events =
        (expected_len + FIO___TLS13_OPENSSL_RT_PARTIAL_READ - 1) /
        FIO___TLS13_OPENSSL_RT_PARTIAL_READ;
    FIO_ASSERT(fio___tls13_openssl_rt_state.partial_received[phase] ==
                   expected_len,
               "partial-read phase %zu length mismatch: %zu != %zu",
               phase,
               fio___tls13_openssl_rt_state.partial_received[phase],
               expected_len);
    if (!phase) {
      FIO_ASSERT(fio___tls13_openssl_rt_state.partial_on_data[phase] ==
                     expected_events,
                 "single-record phase on_data count mismatch: %zu != %zu",
                 fio___tls13_openssl_rt_state.partial_on_data[phase],
                 expected_events);
    } else {
      /* TLS record boundaries can add a short read, so only the lower bound is
       * stable across OpenSSL versions and record-splitting policies. */
      FIO_ASSERT(fio___tls13_openssl_rt_state.partial_on_data[phase] >=
                     expected_events,
                 "multi-record phase on_data count too small: %zu < %zu",
                 fio___tls13_openssl_rt_state.partial_on_data[phase],
                 expected_events);
    }
  }

  fprintf(stderr,
          "TLS 1.3 HTTP, partial-read, and peer-certificate roundtrips "
          "passed.\n");
  return 0;
}

#else /* !HAVE_OPENSSL || !FIO_OS_POSIX */

int main(void) {
  fprintf(stderr,
          "* TLS 1.3 OpenSSL roundtrip skipped (OpenSSL/POSIX unavailable).\n");
  return 0;
}

#endif /* HAVE_OPENSSL && FIO_OS_POSIX */
