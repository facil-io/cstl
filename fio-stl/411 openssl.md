## OpenSSL TLS Integration

```c
#define FIO_IO
#include "fio-stl/include.h"
```

The OpenSSL module provides TLS (Transport Layer Security) integration for the facil.io IO reactor using OpenSSL 3.x. When OpenSSL is available, this module automatically registers itself as the default TLS implementation.

**Note**: this module requires OpenSSL 3.x or later. It will not compile if `FIO_NO_TLS` is defined or if OpenSSL is unavailable.

**Note**: this module is automatically included when `FIO_IO` is defined and OpenSSL headers are detected (via `HAVE_OPENSSL` or `__has_include("openssl/ssl.h")`).

### Conditional Compilation

The OpenSSL module compiles only when all of the following conditions are met:

- `FIO_IO` is defined (the IO reactor module is included)
- `FIO_NO_TLS` is **not** defined
- OpenSSL 3.x headers are available (`HAVE_OPENSSL` defined or `openssl/ssl.h` exists)

If OpenSSL is detected but the version is older than 3.x, a compiler warning is issued and the module falls back to the default (no-op) TLS functions.

### Features

The OpenSSL integration provides:

- **TLS 1.3 Support**: Automatic TLS protocol negotiation via OpenSSL
- **Self-Signed Certificates**: Automatic generation using ECDSA P-256 for development/testing
- **Certificate Loading**: Load certificates and private keys from PEM files
- **ALPN Protocol Negotiation**: Application-Layer Protocol Negotiation for HTTP/2, etc.
- **Certificate Verification**: Peer verification with configurable trust store
- **Non-Blocking I/O**: Seamless integration with the facil.io event-driven IO reactor

### Usage with the IO Reactor

The OpenSSL module integrates with the IO system through the `fio_io_tls_s` configuration object:

```c
#define FIO_IO
#include "fio-stl.h"

/* Create a TLS configuration object */
fio_io_tls_s *tls = fio_io_tls_new();

/* Optional: load certificates from PEM files */
fio_io_tls_cert_add(tls,
                    "www.example.com",  /* server name (SNI) */
                    "cert.pem",         /* public certificate */
                    "key.pem",          /* private key */
                    NULL);              /* password (if key is encrypted) */

/* Optional: add trusted CA certificates for peer verification */
fio_io_tls_trust_add(tls, "ca.pem");

/* Optional: configure ALPN protocol negotiation */
fio_io_tls_alpn_add(tls, "h2", on_http2_selected);
fio_io_tls_alpn_add(tls, "http/1.1", on_http1_selected);

/* Start listening with TLS */
fio_io_listen(.url = "0.0.0.0:443",
              .protocol = &MY_PROTOCOL,
              .tls = tls);

/* The TLS object can be freed after fio_io_listen (it's reference counted) */
fio_io_tls_free(tls);

/* Start the IO reactor */
fio_io_start(0);
```

### HTTPS Server Example

A complete example of an HTTPS server:

```c
#define FIO_LOG
#define FIO_IO
#include "fio-stl.h"

/* Protocol callbacks */
FIO_SFUNC void on_data(fio_io_s *io) {
  char buf[1024];
  size_t len = fio_io_read(io, buf, sizeof(buf));
  if (len) {
    /* Echo back with HTTP response */
    const char response[] = 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 13\r\n"
        "Connection: close\r\n"
        "\r\n"
        "Hello, TLS!\n";
    fio_io_write(io, response, sizeof(response) - 1);
    fio_io_close(io);
  }
}

fio_io_protocol_s HTTPS_PROTOCOL = {
    .on_data = on_data,
    .on_timeout = fio_io_touch,
};

int main(void) {
  /* Create TLS context - will use self-signed certificate */
  fio_io_tls_s *tls = fio_io_tls_new();
  
  /* For production, load real certificates:
   * fio_io_tls_cert_add(tls, "example.com", "cert.pem", "key.pem", NULL);
   */
  
  fio_io_listen(.url = "0.0.0.0:8443",
                .protocol = &HTTPS_PROTOCOL,
                .tls = tls);
  
  fio_io_tls_free(tls);
  
  FIO_LOG_INFO("HTTPS server listening on port 8443");
  FIO_LOG_INFO("Test with: curl -k https://localhost:8443/");
  
  fio_io_start(0);
  return 0;
}
```

### Self-Signed Certificates

When no certificate is configured for a server, the OpenSSL module automatically generates a self-signed certificate with the following properties:

| Property | Value |
|----------|-------|
| Algorithm | ECDSA with P-256 curve |
| Security Level | 128-bit (equivalent to RSA-3072) |
| Signature | SHA-256 |
| Validity | 180 days |
| Serial Number | 128-bit cryptographically random |
| Key Generation | ~10ms (vs ~2000ms for RSA-4096) |

**X.509v3 Extensions** (for browser compatibility):

- **Basic Constraints**: `CA:FALSE` (not a CA certificate)
- **Key Usage**: `digitalSignature`, `keyEncipherment`
- **Extended Key Usage**: `serverAuth`
- **Subject Alternative Name (SAN)**: DNS name matching the server name

**Note**: Self-signed certificates are intended for development and testing only. Browsers will show security warnings. Use properly issued certificates from a trusted Certificate Authority (CA) in production.

### API Reference

#### `fio_openssl_io_functions`

```c
fio_io_functions_s fio_openssl_io_functions(void);
```

Returns the OpenSSL IO functions structure for TLS operations.

This function is called automatically during module initialization to register OpenSSL as the default TLS implementation. You typically don't need to call this directly.

**Returns:** A `fio_io_functions_s` structure containing:

- `build_context` - Creates an SSL_CTX from `fio_io_tls_s` configuration
- `free_context` - Frees the SSL_CTX and associated resources
- `start` - Initializes TLS for a new connection (SSL_new, handshake)
- `read` - Non-blocking TLS read (SSL_read)
- `write` - Non-blocking TLS write (SSL_write)
- `flush` - Flushes pending TLS data (no-op for OpenSSL)
- `finish` - Initiates TLS shutdown (SSL_shutdown)
- `cleanup` - Frees per-connection SSL object

### TLS Configuration Functions

The following functions from the IO module are used to configure TLS. See the [IO Reactor documentation](400%20io.md) for complete details.

#### `fio_io_tls_new`

```c
fio_io_tls_s *fio_io_tls_new(void);
```

Creates a new TLS configuration object.

#### `fio_io_tls_free`

```c
void fio_io_tls_free(fio_io_tls_s *tls);
```

Frees a TLS configuration object (reference counted).

#### `fio_io_tls_cert_add`

```c
fio_io_tls_s *fio_io_tls_cert_add(fio_io_tls_s *tls,
                                  const char *server_name,
                                  const char *public_cert_file,
                                  const char *private_key_file,
                                  const char *pk_password);
```

Adds a certificate to the TLS context. Supports SNI (Server Name Indication) for hosting multiple domains.

- `server_name` - The server name for SNI matching
- `public_cert_file` - Path to PEM-encoded certificate (or certificate chain)
- `private_key_file` - Path to PEM-encoded private key
- `pk_password` - Password for encrypted private keys (or NULL)

If `public_cert_file` and `private_key_file` are both NULL, a self-signed certificate is generated.

#### `fio_io_tls_trust_add`

```c
fio_io_tls_s *fio_io_tls_trust_add(fio_io_tls_s *tls,
                                   const char *public_cert_file);
```

Adds a trusted CA certificate for peer verification.

- `public_cert_file` - Path to PEM-encoded CA certificate, or NULL to use system defaults

When trust certificates are added, peer verification is enabled (`SSL_VERIFY_PEER`).

#### `fio_io_tls_alpn_add`

```c
fio_io_tls_s *fio_io_tls_alpn_add(fio_io_tls_s *tls,
                                  const char *protocol_name,
                                  void (*on_selected)(fio_io_s *));
```

Registers an ALPN protocol and its selection callback.

- `protocol_name` - Protocol identifier (e.g., "h2", "http/1.1")
- `on_selected` - Callback invoked when this protocol is negotiated

The first protocol added is the preferred/default protocol.

### Security Considerations

#### Certificate Verification

- **Server Mode**: Certificate verification is typically not enabled (clients don't usually present certificates)

- **Client Mode**: If no trust store is configured, verification is disabled with a security warning logged

```c
/* Enable certificate verification for client connections */
fio_io_tls_s *tls = fio_io_tls_new();
fio_io_tls_trust_add(tls, NULL);  /* Use system trust store */
/* or */
fio_io_tls_trust_add(tls, "ca-bundle.pem");  /* Use specific CA */
```

#### Production Recommendations

1. **Use Real Certificates**: Obtain certificates from a trusted CA (e.g., Let's Encrypt)
2. **Enable Verification**: Always configure trust stores for client connections
3. **Keep OpenSSL Updated**: Security patches are released regularly
4. **Protect Private Keys**: Use appropriate file permissions and consider encrypted keys

#### SIGPIPE Handling

The module automatically monitors `SIGPIPE` signals to prevent OpenSSL from crashing the application when writing to closed connections.

### Non-Blocking I/O Integration

The OpenSSL module configures SSL contexts for non-blocking operation:

- `SSL_MODE_ENABLE_PARTIAL_WRITE` - Allow partial writes
- `SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER` - Buffer can move between writes
- `SSL_MODE_AUTO_RETRY` disabled - Return immediately on would-block

TLS handshakes are performed asynchronously:
- Server connections use `SSL_accept()`
- Client connections use `SSL_connect()`

Both return immediately and complete during subsequent read/write operations.

### Error Handling

OpenSSL errors are logged using the facil.io logging system:

- `FIO_LOG_ERROR` - Critical failures (certificate loading, key generation)
- `FIO_LOG_WARNING` - Non-fatal issues (trust store loading)
- `FIO_LOG_SECURITY` - Security-relevant warnings (verification disabled)
- `FIO_LOG_DEBUG2` - Detailed debugging information

### Memory Management

- SSL contexts (`SSL_CTX`) are reference counted and shared across connections
- Per-connection SSL objects are allocated on connection start and freed on close
- The global ECDSA private key (for self-signed certificates) is freed at program exit
- Context cleanup is deferred to avoid blocking the IO reactor

------------------------------------------------------------
