/* *****************************************************************************
Shared TLS test - fio_io_connect `.verify` over loopback (opt-in system trust)

Include after FIO_INCLUDE_FILE (with FIO_IO and a TLS backend), then call
`fio___test_tls_connect_verify(backend_io_functions, "name")`. POSIX only.

The server presents a self-signed certificate for `localhost` and clients
connect to `localhost` (resolved to 127.0.0.1 by `fio_sock_open`), so the
identity check passes and only trust is in question:

- [0] no verify: the handshake completes (control);
- [1] `.verify = 1` (system trust): the untrusted certificate is rejected;
- [2] `.verify = 1` with an explicit (missing) trust file: kept, rejected;
- [3] `.verify = 1` trusting the server certificate: the handshake completes,
  proving [1] fails the trust check, not the host name check.

The test runs twice: with the self-signed P-256 certificate (trusted as is),
and with a CA-issued P-256 chain whose points are compressed (SEC 1 02/03 || x;
the server sends leaf + CA, [3] trusts the CA), so both the handshake
signature and chain verification use compressed public keys.
***************************************************************************** */
#ifndef H___FIO_TEST_TLS_CONNECT_VERIFY_H
#define H___FIO_TEST_TLS_CONNECT_VERIFY_H
#if FIO_OS_POSIX
#include <netinet/in.h>
#include <sys/socket.h>

#include "tls-pem-load.h" /* certificate fixtures */

#define FIO___TEST_TLSV_N 4

static char fio___test_tlsv_url[64];
/* closed / failed and handshake completed, per udata */
static volatile int fio___test_tlsv_done[FIO___TEST_TLSV_N];
static volatile int fio___test_tlsv_alpn[FIO___TEST_TLSV_N];
static int fio___test_tlsv_ticks;
static int fio___test_tlsv_step;
static const char *fio___test_tlsv_name;
static fio_io_protocol_s fio___test_tlsv_cli_pr;
static fio_io_tls_s *fio___test_tlsv_tls[FIO___TEST_TLSV_N];

FIO_SFUNC void fio___test_tlsv_on_alpn(fio_io_s *io) {
  uintptr_t i = (uintptr_t)fio_io_udata(io);
  if (i < FIO___TEST_TLSV_N)
    ++fio___test_tlsv_alpn[i];
  fio_io_close(io);
}
FIO_SFUNC void fio___test_tlsv_on_attach(fio_io_s *io) {
  fio_io_write(io, "ping", 4); /* pending output drives the handshake */
}
FIO_SFUNC void fio___test_tlsv_on_data(fio_io_s *io) {
  char buf[64];
  while (fio_io_read(io, buf, sizeof(buf)))
    ;
}
FIO_SFUNC void fio___test_tlsv_on_close(void *b, void *u) {
  if ((uintptr_t)u < FIO___TEST_TLSV_N)
    ++fio___test_tlsv_done[(uintptr_t)u];
  (void)b;
}
FIO_SFUNC void fio___test_tlsv_on_failed(fio_io_protocol_s *p, void *u) {
  fio___test_tlsv_on_close(NULL, u);
  (void)p;
}

FIO_SFUNC int fio___test_tlsv_driver(void *u1, void *u2) {
  (void)u1, (void)u2;
  if (++fio___test_tlsv_ticks > 1000) { /* ~10 seconds */
    fio_io_stop();
    FIO_ASSERT(0,
               "%s .verify loopback test timed out (done %d%d%d%d, "
               "handshakes %d%d%d%d)",
               fio___test_tlsv_name,
               fio___test_tlsv_done[0],
               fio___test_tlsv_done[1],
               fio___test_tlsv_done[2],
               fio___test_tlsv_done[3],
               fio___test_tlsv_alpn[0],
               fio___test_tlsv_alpn[1],
               fio___test_tlsv_alpn[2],
               fio___test_tlsv_alpn[3]);
  }
  if (!fio___test_tlsv_step) {
    fio___test_tlsv_step = 1;
    for (uintptr_t i = 0; i < FIO___TEST_TLSV_N; ++i)
      fio_io_connect(fio___test_tlsv_url,
                     .protocol = &fio___test_tlsv_cli_pr,
                     .on_failed = fio___test_tlsv_on_failed,
                     .udata = (void *)i,
                     .tls = fio_io_tls_dup(fio___test_tlsv_tls[i]), /* kept */
                     .timeout = 2000,
                     .verify = (uint8_t)(i != 0));
    return 0;
  }
  if (fio___test_tlsv_done[0] && fio___test_tlsv_done[1] &&
      fio___test_tlsv_done[2] && fio___test_tlsv_done[3]) {
    fio_io_stop();
    return -1;
  }
  return 0;
}

/** One run: the server uses `cert` / `key` files, client [3] trusts `trust`. */
FIO_SFUNC void fio___test_tlsv_run(fio_io_functions_s fn,
                                   const char *name,
                                   const char *cert,
                                   const char *key,
                                   const char *trust) {
  fio___test_tlsv_name = name;
  fio___test_tlsv_ticks = 0;
  fio___test_tlsv_step = 0;
  for (size_t i = 0; i < FIO___TEST_TLSV_N; ++i)
    fio___test_tlsv_done[i] = fio___test_tlsv_alpn[i] = 0;
  char listen_url[64];
  { /* reserve a loopback port */
    fio_socket_i r =
        fio_sock_open2("tcp://127.0.0.1:0", FIO_SOCK_SERVER | FIO_SOCK_TCP);
    FIO_ASSERT(FIO_SOCK_FD_ISVALID(r), "failed to reserve loopback port");
    struct sockaddr_in a = {0};
    socklen_t alen = sizeof(a);
    FIO_ASSERT(!getsockname(r, (struct sockaddr *)&a, &alen), "getsockname");
    fio_sock_close(r);
    /* clients use the certificate's name; the server binds 127.0.0.1, which
     * `fio_sock_open` always resolves "localhost" to (never ::1). */
    snprintf(fio___test_tlsv_url,
             sizeof(fio___test_tlsv_url),
             "tls://localhost:%u",
             (unsigned)ntohs(a.sin_port));
    snprintf(listen_url,
             sizeof(listen_url),
             "tls://127.0.0.1:%u",
             (unsigned)ntohs(a.sin_port));
  }
  static fio_io_protocol_s server_pr;
  server_pr = (fio_io_protocol_s){.on_data = fio___test_tlsv_on_data,
                                  .on_timeout = fio_io_touch,
                                  .io_functions = fn};
  fio___test_tlsv_cli_pr =
      (fio_io_protocol_s){.on_attach = fio___test_tlsv_on_attach,
                          .on_data = fio___test_tlsv_on_data,
                          .on_close = fio___test_tlsv_on_close,
                          .timeout = 1000, /* bounds close */
                          .io_functions = fn};
  fio_io_tls_s *server_tls = fio_io_tls_new(); /* self-signed "localhost" */
  fio_io_tls_cert_add(server_tls, "localhost", cert, key, NULL);
  fio_io_tls_alpn_add(server_tls, "y", NULL);
  for (size_t i = 0; i < FIO___TEST_TLSV_N; ++i) {
    fio___test_tlsv_tls[i] = fio_io_tls_new();
    fio_io_tls_alpn_add(fio___test_tlsv_tls[i], "y", fio___test_tlsv_on_alpn);
  }
  /* [0]: control (no verify); [1]: verify, no trust; [2]: verify, explicit
   * (unrelated, missing) trust file - kept, so the handshake still fails. */
  fio_io_tls_trust_add(fio___test_tlsv_tls[2], "tmp/fio-missing-trust.pem");
  /* [3]: verify, trusting the server's certificate or CA (identity control). */
  fio_io_tls_trust_add(fio___test_tlsv_tls[3], trust);

  int log_level = FIO_LOG_LEVEL_GET();
  FIO_LOG_LEVEL_SET(FIO_LOG_LEVEL_FATAL); /* expected handshake errors */
  fio_io_listener_s *l = fio_io_listen(.url = listen_url,
                                       .protocol = &server_pr,
                                       .tls = server_tls,
                                       .hide_from_log = 1);
  FIO_ASSERT(l, "TLS listen failed");
  fio_io_run_every(.fn = fio___test_tlsv_driver,
                   .every = 10,
                   .repetitions = -1);
  fio_io_start(0);
  FIO_LOG_LEVEL_SET(log_level);
  fio_io_listen_stop(l); /* the listener owned `server_tls` */

  FIO_ASSERT(fio___test_tlsv_alpn[0] == 1,
             "%s: control TLS client should complete the handshake",
             name);
  FIO_ASSERT(fio___test_tlsv_alpn[3] == 1,
             "%s: .verify trusting the server certificate (or its CA) should "
             "complete the handshake to \"localhost\" (identity check passes)",
             name);
  FIO_ASSERT(!fio___test_tlsv_alpn[1],
             "%s: .verify must reject an untrusted server",
             name);
  FIO_ASSERT(!fio___test_tlsv_alpn[2],
             "%s: .verify with explicit trust must reject an untrusted server",
             name);
  FIO_ASSERT(!fio___test_tlsv_tls[0]->trust_sys,
             "no verify: system trust must not be added");
  FIO_ASSERT(fio___test_tlsv_tls[1]->trust_sys &&
                 !fio_io_tls_trust_count(fio___test_tlsv_tls[1]),
             ".verify without trust should add the system trust store");
  FIO_ASSERT(!fio___test_tlsv_tls[2]->trust_sys &&
                 fio_io_tls_trust_count(fio___test_tlsv_tls[2]) == 1,
             ".verify must keep (not replace) explicit trust entries");
  for (size_t i = 0; i < FIO___TEST_TLSV_N; ++i)
    fio_io_tls_free(fio___test_tlsv_tls[i]);
}

FIO_SFUNC void fio___test_tls_connect_verify(fio_io_functions_s fn,
                                             const char *name) {
  char cert[64], key[64], cc[64], ck[64], ca[64], desc[96];
  fio___test_tls_pem_file(cert, sizeof(cert), "vcert", fio___test_tls_pem_cert);
  fio___test_tls_pem_file(key, sizeof(key), "vkey", fio___test_tls_pem_key);
  fio___test_tls_pem_chain(cc,
                           sizeof(cc),
                           "vcc",
                           fio___test_ecc_p256_leaf,
                           fio___test_ecc_p256_ca);
  fio___test_tls_pem_file(ck, sizeof(ck), "vck", fio___test_ecc_p256_key);
  fio___test_tls_pem_file(ca, sizeof(ca), "vca", fio___test_ecc_p256_ca);
  fio___test_tlsv_run(fn, name, cert, key, cert);
  snprintf(desc, sizeof(desc), "%s (compressed P-256 chain)", name);
  fio___test_tlsv_run(fn, desc, cc, ck, ca);
  unlink(cert);
  unlink(key);
  unlink(cc);
  unlink(ck);
  unlink(ca);
  fprintf(stderr, "* %s fio_io_connect .verify: OK\n", name);
}
#else
FIO_SFUNC void fio___test_tls_connect_verify(fio_io_functions_s fn,
                                             const char *name) {
  (void)fn, (void)name;
}
#endif
#endif /* H___FIO_TEST_TLS_CONNECT_VERIFY_H */
