# OpenSSL TLS Backend

```c
#define FIO_IO
#include "fio-stl/include.h"
```

OpenSSL 3.x IO-layer TLS backend for the facil.io reactor. When OpenSSL is
available at build time this module auto-registers itself as the default TLS
transport. You don't have to call anything — just define `FIO_IO` and link
against OpenSSL 3.x.

For IO-layer TLS context overview see [./400 io-overview.md](./400 io-overview.md).  
For the sibling native TLS 1.3 backend see [./405 tls13.md](./405 tls13.md) *(planned)*.  
For standalone TLS 1.3 crypto (key schedule, record layer) see [./190 tls13.md](./190 tls13.md).

---

## Activation

The module compiles automatically when all of the following are true:

| Condition | Notes |
|-----------|-------|
| `FIO_IO` is defined | pulls in the IO reactor |
| `FIO_NO_TLS` is **not** defined | opt-out guard |
| OpenSSL 3.x headers are present | `HAVE_OPENSSL` or `__has_include("openssl/ssl.h")` |

If OpenSSL headers are found but the version is older than 3.x
(`OPENSSL_VERSION_MAJOR < 3`), the module emits a compiler warning and
makes `fio_openssl_io_functions()` return `fio_io_tls_default_functions(NULL)`
(the current default, usually a no-op). Everything compiles; TLS simply does nothing.

A module-level constructor runs before `main()`:

- Initialises the custom BIO methods.
- Calls `fio_io_tls_default_functions(&openssl_io_funcs)` so every subsequent
  `fio_io_listen` / `fio_io_connect` call with a `tls` argument uses OpenSSL.
- Registers a `SIGPIPE` monitor (OpenSSL can trigger SIGPIPE on broken sockets).
- Registers a `FIO_CALL_AT_EXIT` cleanup for the custom BIO method objects.

---

## Public API

### `fio_openssl_io_functions`

```c
fio_io_functions_s fio_openssl_io_functions(void);
```

Returns the `fio_io_functions_s` vtable that wires OpenSSL into the IO reactor:

| Field | Role |
|-------|------|
| `build_context` | Converts `fio_io_tls_s` into an `SSL_CTX` wrapper |
| `free_context` | Deferred free of `SSL_CTX` and the `fio_io_tls_s` reference |
| `start` | Per-connection: `SSL_new`, custom BIO setup, initial handshake |
| `read` | Non-blocking decrypt: advances handshake if needed, then `SSL_read_ex` |
| `write` | Non-blocking encrypt: `SSL_write_ex` → encrypt → socket |
| `flush` | Sends any pending encrypted bytes from the internal buffer |
| `finish` | Sends TLS `close_notify` before the TCP close |
| `cleanup` | Frees the per-connection `SSL` object |

Normally you never call this directly — the constructor handles registration.
Use it explicitly only when overriding the default or building a custom setup:

```c
/* Override: switch back to OpenSSL after something else changed the default */
fio_io_functions_s openssl_funcs = fio_openssl_io_functions();
fio_io_tls_default_functions(&openssl_funcs);
```

Or to set it on a specific protocol without touching the global default
(the `io_functions` field lives on `fio_io_protocol_s`):

```c
fio_io_functions_s openssl_funcs = fio_openssl_io_functions();
MY_PROTOCOL.io_functions = openssl_funcs; /* per-protocol override */

fio_io_listen(.url = "0.0.0.0:8443",
              .protocol = &MY_PROTOCOL,
              .tls = tls);
```

---

## Configuring TLS — `fio_io_tls_s`

TLS parameters are held in a `fio_io_tls_s` object defined in `401 io api.h`.
Build one before calling `fio_io_listen` or `fio_io_connect`:

```c
/* Allocate (reference counted) */
fio_io_tls_s *tls = fio_io_tls_new();

/* Certificate — PEM files */
fio_io_tls_cert_add(tls,
    "www.example.com",   /* server_name (SNI) */
    "cert.pem",          /* public certificate or chain */
    "key.pem",           /* private key */
    NULL);               /* PEM password, or NULL */

/* ALPN protocol negotiation */
fio_io_tls_alpn_add(tls, "h2",       on_http2_selected);
fio_io_tls_alpn_add(tls, "http/1.1", on_http1_selected);

/* Peer certificate verification */
fio_io_tls_trust_add(tls, NULL);      /* use system trust store */
fio_io_tls_trust_add(tls, "ca.pem"); /* or a specific CA bundle */

/* Listen */
fio_io_listen(.url = "0.0.0.0:443",
              .protocol = &MY_PROTOCOL,
              .tls = tls);

fio_io_tls_free(tls); /* release your reference; the listener holds its own */
fio_io_start(0);
```

`fio_io_tls_s` is reference-counted (`fio_io_tls_dup` / `fio_io_tls_free`).
The backend duplicates the reference during `build_context`; freeing yours
after `fio_io_listen` is always safe.

### URL shorthand

The URL query string can also configure TLS without building the object
manually:

```c
fio_io_listen(.url = "0.0.0.0:443/?tls=./certs/", .protocol = &MY_PROTOCOL);
```

See `fio_io_tls_from_url` in `401 io api.h` for the query syntax.

---

## Certificates

### Loading from PEM files

When `public_cert_file` and `private_key_file` are both provided to
`fio_io_tls_cert_add`, the backend:

1. Sets a PEM password callback if `pk_password` is non-`NULL`.
2. Loads the certificate chain via `SSL_CTX_use_certificate_chain_file`.
3. Loads the private key via `SSL_CTX_use_PrivateKey_file` (PEM format).
4. Verifies key-cert consistency with `SSL_CTX_check_private_key`.

Errors are logged with `FIO_LOG_ERROR` and the context build fails.

### Self-signed fallback

When a **server** has no certificates configured (or `fio_io_tls_cert_add` is
called with both file arguments as `NULL`), the backend generates a self-signed
ECDSA P-256 certificate on the fly:

| Property | Value |
|----------|-------|
| Key algorithm | ECDSA P-256 (128-bit security ≈ RSA-3072) |
| Key generation time | ~10 ms |
| Signature | SHA-256 |
| Validity | 180 days |
| Serial number | 128-bit cryptographically random |
| X.509 extensions | Basic Constraints (`CA:FALSE`), Key Usage, Extended Key Usage (`serverAuth`), SAN |

The private key is generated once per process (thread-safe, lock-protected) and
freed at exit via a `FIO_CALL_AT_EXIT` callback.

Self-signed certificates are fine for development. Browsers will warn. Use a
CA-issued certificate (e.g. Let's Encrypt) in production.

---

## ALPN

Register protocols with `fio_io_tls_alpn_add` before listen/connect. The first
registered protocol is the preferred default.

When a client sends an ALPN extension, the backend walks the offered names in
order and picks the first that matches a registered protocol.  On match it calls
`fio_io_tls_alpn_select`, which fires the `on_selected` callback for that
connection.  If nothing matches, the handshake fails with a fatal alert.

Protocol names must be 1–255 bytes. The internal wire-format list is capped at
1 023 bytes total; more protocols than that will log an overflow error.

---

## Trust and Peer Verification

| Scenario | Behaviour |
|----------|-----------|
| `fio_io_tls_trust_add` called (any argument) | `SSL_VERIFY_PEER` enabled |
| `NULL` passed to `fio_io_tls_trust_add` | system trust store is loaded |
| No trust certs added, client mode | `SSL_VERIFY_NONE` + `FIO_LOG_SECURITY` warning |
| No trust certs added, server mode | `SSL_VERIFY_NONE` (clients rarely present certs) |

---

## SSL Context Modes

The `SSL_CTX` is configured for non-blocking use:

```
SSL_MODE_ENABLE_PARTIAL_WRITE      — SSL_write may return before all data encrypted
SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER — buffer pointer may differ between retries
SSL_MODE_RELEASE_BUFFERS           — free idle 34 KB per-connection OpenSSL buffers
SSL_MODE_AUTO_RETRY                — CLEARED (return immediately on WANT_READ/WRITE)
```

TLS 1.3 session tickets: 2 tickets are configured for session resumption
(saves ~30–50 % of handshake CPU on reconnects).

---

## Internal Design — Custom BIOs

Instead of `BIO_s_mem()` (which bounces data through a private `BUF_MEM`),
the backend uses custom `BIO_METHOD` objects that give OpenSSL direct access to
the per-connection buffers:

- **rbio** — OpenSSL reads raw socket bytes directly from a 64 KB receive
  buffer that was filled by a single `fio_sock_read` call.
- **wbio** — OpenSSL appends encrypted output directly into a ~66 KB
  encrypted-output buffer (4 × max TLS record).

This eliminates all intermediate copies while retaining full control over
buffering and partial writes.

Handshake responses (ServerHello, etc.) are flushed to the socket from inside
the `read` callback — the IO layer's `on_ready` loop doesn't run until the
handshake completes, so the backend must send them itself.

---

## Lifecycle and Error Behavior

```
fio_io_listen / fio_io_connect
    │
    └─► build_context (SSL_CTX built once per listener/connector)
            │
            └─► per-connection start (SSL_new + BIO setup + SSL_accept/SSL_connect)
                    │
                    ├─ read  (advances handshake until SSL_is_init_finished,
                    │         then SSL_read_ex)
                    ├─ write (SSL_write_ex, encrypt into wbio buffer, flush socket)
                    ├─ flush (send remaining enc_buf bytes)
                    │
                    ├─ finish  (SSL_shutdown + best-effort send of close_notify)
                    └─ cleanup (SSL_free — also frees both BIOs)
```

**Error codes** surfaced to the IO layer:

- `return 0` — peer closed cleanly (`SSL_ERROR_ZERO_RETURN`) or fatal SSL error.
- `return -1` with `errno = EWOULDBLOCK` — not enough data yet (normal event-loop
  signalling; the reactor will retry on next readable event).

All non-fatal internal issues are logged via `FIO_LOG_ERROR` /
`FIO_LOG_WARNING` / `FIO_LOG_SECURITY`. Debug-level detail is at
`FIO_LOG_DDEBUG2`.

---

## Minimal Server Example

```c
#define FIO_LOG
#define FIO_IO
#include "fio-stl/include.h"

static void on_data(fio_io_s *io) {
  char buf[4096];
  size_t n = fio_io_read(io, buf, sizeof(buf));
  if (n)
    fio_io_write(io, buf, n); /* echo */
}

static fio_io_protocol_s ECHO_PROTO = {
    .on_data    = on_data,
    .on_timeout = fio_io_touch,
};

int main(void) {
  /* No certificate configured → self-signed ECDSA P-256 generated automatically */
  fio_io_tls_s *tls = fio_io_tls_new();

  fio_io_listen(.url      = "0.0.0.0:8443",
                .protocol = &ECHO_PROTO,
                .tls      = tls);
  fio_io_tls_free(tls);

  FIO_LOG_INFO("TLS echo server on :8443  (test: openssl s_client -connect localhost:8443)");
  fio_io_start(0);
}
```

For a production server, load real certificates:

```c
fio_io_tls_cert_add(tls, "example.com", "cert.pem", "key.pem", NULL);
```

---

## Disambiguation

| Document | Scope |
|----------|-------|
| **This document** | OpenSSL 3.x IO-layer backend — plugs TLS into the reactor |
| [./405 tls13.md](./405 tls13.md) *(planned)* | Native TLS 1.3 IO backend (same `fio_io_functions_s` interface, no OpenSSL dependency) |
| [./190 tls13.md](./190 tls13.md) | Standalone TLS 1.3 crypto library (key schedule, record layer, raw machinery) |
| [./400 io-overview.md](./400 io-overview.md) | IO + TLS stack overview |
