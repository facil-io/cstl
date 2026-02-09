## BLAKE2

```c
#define FIO_BLAKE2
#include FIO_INCLUDE_FILE
```

By defining `FIO_BLAKE2`, the BLAKE2 cryptographic hash functions are defined and made available. This module provides both BLAKE2b (64-bit optimized, up to 64-byte digest) and BLAKE2s (32-bit optimized, up to 32-byte digest).

BLAKE2 is a high-speed cryptographic hash function standardized in [RFC 7693](https://tools.ietf.org/html/rfc7693). It is faster than MD5, SHA-1, and SHA-2 while providing security comparable to SHA-3.

Each variant supports:

- **One-shot hashing** - hash data in a single call, returning a fixed-size result type
- **Flexible-output hashing** - hash with configurable digest length and optional keying
- **HMAC construction** - standard HMAC (RFC 2104) using BLAKE2 as the underlying hash
- **Streaming** - init/consume/finalize pattern for incremental hashing

### BLAKE2b Types

#### `fio_blake2b_s`

```c
typedef struct {
  uint64_t h[8];    /* state */
  uint64_t t[2];    /* total bytes processed (128-bit counter) */
  uint64_t f[2];    /* finalization flags */
  uint8_t buf[128]; /* input buffer */
  size_t buflen;    /* bytes in buffer */
  size_t outlen;    /* digest length */
} fio_blake2b_s;
```

Streaming BLAKE2b context (64-bit, up to 64-byte digest).

**Members:**
- `h` - 8-word internal state (512 bits)
- `t` - 128-bit byte counter (total bytes processed)
- `f` - finalization flags
- `buf` - 128-byte input buffer for partial blocks
- `buflen` - number of bytes currently in the buffer
- `outlen` - requested digest length in bytes (1-64)

**Note**: this type should be treated as opaque. Use `fio_blake2b_init` to initialize it.

### BLAKE2b Functions

#### `fio_blake2b`

```c
fio_u512 fio_blake2b(const void *data, uint64_t len);
```

One-shot BLAKE2b hash with maximum-length (64-byte) digest.

Returns the hash as a `fio_u512` value. All 64 bytes of the result are valid.

**Parameters:**
- `data` - pointer to the data to hash
- `len` - length of the data in bytes

**Returns:** a `fio_u512` containing the 64-byte BLAKE2b digest.

**Note**: this function has the same signature as `fio_sha512`, making them interchangeable as function pointers of type `fio_u512 (*)(const void *, uint64_t)`.

#### `fio_blake2b_hash`

```c
void fio_blake2b_hash(void *restrict out,
                      size_t outlen,
                      const void *restrict data,
                      size_t len,
                      const void *restrict key,
                      size_t keylen);
```

Flexible-output BLAKE2b hash with optional keying.

Computes a BLAKE2b hash of `data` with a configurable output length and optional secret key. The result is written to `out`.

**Parameters:**
- `out` - destination buffer (must have capacity for at least `outlen` bytes)
- `outlen` - desired digest length in bytes (1-64; defaults to 64 if 0)
- `data` - pointer to the data to hash
- `len` - length of the data in bytes
- `key` - optional secret key for keyed hashing (NULL for unkeyed)
- `keylen` - length of the key in bytes (0-64; clamped to 64 if larger)

**Note**: keyed BLAKE2b is a built-in MAC mode defined by the BLAKE2 specification. It differs from HMAC-BLAKE2b (see `fio_blake2b_hmac`).

#### `fio_blake2b_hmac`

```c
fio_u512 fio_blake2b_hmac(const void *key,
                          uint64_t key_len,
                          const void *msg,
                          uint64_t msg_len);
```

Computes HMAC-BLAKE2b using the standard HMAC construction (RFC 2104), producing a 64-byte authentication code.

Uses a 128-byte block size (BLAKE2b's internal block size). If the key exceeds 128 bytes, it is first hashed with BLAKE2b. Intermediate key material is securely zeroed after use.

**Parameters:**
- `key` - pointer to the secret key
- `key_len` - length of the key in bytes
- `msg` - pointer to the message to authenticate
- `msg_len` - length of the message in bytes

**Returns:** a `fio_u512` containing the 64-byte HMAC authentication code.

**Note**: this uses the standard HMAC construction `H((K ^ opad) || H((K ^ ipad) || msg))`, which is different from BLAKE2b's built-in keyed mode. Use this when interoperability with standard HMAC implementations is required.

#### `fio_blake2b_init`

```c
fio_blake2b_s fio_blake2b_init(size_t outlen,
                               const void *key,
                               size_t keylen);
```

Initializes a BLAKE2b streaming context.

**Parameters:**
- `outlen` - desired digest length in bytes (1-64; defaults to 64 if 0; clamped to 64 if larger)
- `key` - optional secret key for keyed hashing (NULL for unkeyed)
- `keylen` - length of the key in bytes (0-64; clamped to 64 if larger)

**Returns:** an initialized `fio_blake2b_s` context.

#### `fio_blake2b_consume`

```c
void fio_blake2b_consume(fio_blake2b_s *restrict h,
                         const void *restrict data,
                         size_t len);
```

Feeds data into a BLAKE2b streaming context.

Can be called multiple times to incrementally hash data. Internally buffers partial blocks and processes complete 128-byte blocks as they become available.

**Parameters:**
- `h` - pointer to an initialized `fio_blake2b_s` context
- `data` - pointer to the data to consume
- `len` - length of the data in bytes

#### `fio_blake2b_finalize`

```c
fio_u512 fio_blake2b_finalize(fio_blake2b_s *h);
```

Finalizes a BLAKE2b streaming context and returns the digest.

Pads the remaining buffer with zeros, performs the final compression with the finalization flag set, and outputs the hash in little-endian byte order.

**Parameters:**
- `h` - pointer to the streaming context to finalize

**Returns:** a `fio_u512` containing the digest. Only the first `outlen` bytes (as specified during `fio_blake2b_init`) are valid.

**Note**: the context should not be used after finalization.

### BLAKE2s Types

#### `fio_blake2s_s`

```c
typedef struct {
  uint32_t h[8];   /* state */
  uint32_t t[2];   /* total bytes processed (64-bit counter) */
  uint32_t f[2];   /* finalization flags */
  uint8_t buf[64]; /* input buffer */
  size_t buflen;   /* bytes in buffer */
  size_t outlen;   /* digest length */
} fio_blake2s_s;
```

Streaming BLAKE2s context (32-bit, up to 32-byte digest).

**Members:**
- `h` - 8-word internal state (256 bits)
- `t` - 64-bit byte counter (total bytes processed)
- `f` - finalization flags
- `buf` - 64-byte input buffer for partial blocks
- `buflen` - number of bytes currently in the buffer
- `outlen` - requested digest length in bytes (1-32)

**Note**: this type should be treated as opaque. Use `fio_blake2s_init` to initialize it.

### BLAKE2s Functions

#### `fio_blake2s`

```c
fio_u256 fio_blake2s(const void *data, uint64_t len);
```

One-shot BLAKE2s hash with maximum-length (32-byte) digest.

Returns the hash as a `fio_u256` value. All 32 bytes of the result are valid.

**Parameters:**
- `data` - pointer to the data to hash
- `len` - length of the data in bytes

**Returns:** a `fio_u256` containing the 32-byte BLAKE2s digest.

#### `fio_blake2s_hash`

```c
void fio_blake2s_hash(void *restrict out,
                      size_t outlen,
                      const void *restrict data,
                      size_t len,
                      const void *restrict key,
                      size_t keylen);
```

Flexible-output BLAKE2s hash with optional keying.

Computes a BLAKE2s hash of `data` with a configurable output length and optional secret key. The result is written to `out`.

**Parameters:**
- `out` - destination buffer (must have capacity for at least `outlen` bytes)
- `outlen` - desired digest length in bytes (1-32; defaults to 32 if 0)
- `data` - pointer to the data to hash
- `len` - length of the data in bytes
- `key` - optional secret key for keyed hashing (NULL for unkeyed)
- `keylen` - length of the key in bytes (0-32; clamped to 32 if larger)

#### `fio_blake2s_hmac`

```c
fio_u256 fio_blake2s_hmac(const void *key,
                          uint64_t key_len,
                          const void *msg,
                          uint64_t msg_len);
```

Computes HMAC-BLAKE2s using the standard HMAC construction (RFC 2104), producing a 32-byte authentication code.

Uses a 64-byte block size (BLAKE2s's internal block size). If the key exceeds 64 bytes, it is first hashed with BLAKE2s. Intermediate key material is securely zeroed after use.

**Parameters:**
- `key` - pointer to the secret key
- `key_len` - length of the key in bytes
- `msg` - pointer to the message to authenticate
- `msg_len` - length of the message in bytes

**Returns:** a `fio_u256` containing the 32-byte HMAC authentication code.

#### `fio_blake2s_init`

```c
fio_blake2s_s fio_blake2s_init(size_t outlen,
                               const void *key,
                               size_t keylen);
```

Initializes a BLAKE2s streaming context.

**Parameters:**
- `outlen` - desired digest length in bytes (1-32; defaults to 32 if 0; clamped to 32 if larger)
- `key` - optional secret key for keyed hashing (NULL for unkeyed)
- `keylen` - length of the key in bytes (0-32; clamped to 32 if larger)

**Returns:** an initialized `fio_blake2s_s` context.

#### `fio_blake2s_consume`

```c
void fio_blake2s_consume(fio_blake2s_s *restrict h,
                         const void *restrict data,
                         size_t len);
```

Feeds data into a BLAKE2s streaming context.

Can be called multiple times to incrementally hash data. Internally buffers partial blocks and processes complete 64-byte blocks as they become available.

**Parameters:**
- `h` - pointer to an initialized `fio_blake2s_s` context
- `data` - pointer to the data to consume
- `len` - length of the data in bytes

#### `fio_blake2s_finalize`

```c
fio_u256 fio_blake2s_finalize(fio_blake2s_s *h);
```

Finalizes a BLAKE2s streaming context and returns the digest.

Pads the remaining buffer with zeros, performs the final compression with the finalization flag set, and outputs the hash in little-endian byte order.

**Parameters:**
- `h` - pointer to the streaming context to finalize

**Returns:** a `fio_u256` containing the digest. Only the first `outlen` bytes (as specified during `fio_blake2s_init`) are valid.

**Note**: the context should not be used after finalization.

### BLAKE2 Examples

#### One-Shot Hashing

```c
#define FIO_BLAKE2
#include FIO_INCLUDE_FILE

void example_oneshot(void) {
  const char *msg = "Hello, BLAKE2!";
  
  /* BLAKE2b - 64-byte digest */
  fio_u512 b2b = fio_blake2b(msg, strlen(msg));
  /* b2b.u8[0..63] contains the digest */

  /* BLAKE2s - 32-byte digest */
  fio_u256 b2s = fio_blake2s(msg, strlen(msg));
  /* b2s.u8[0..31] contains the digest */
}
```

#### Flexible-Output with Keying

```c
void example_keyed(void) {
  const char *key = "my-secret-key";
  const char *data = "message to hash";
  uint8_t digest[16]; /* 16-byte digest */

  fio_blake2b_hash(digest, 16,
                   data, strlen(data),
                   key, strlen(key));
  /* digest[0..15] contains the 16-byte keyed hash */
}
```

#### Streaming (Incremental) Hashing

```c
void example_streaming(void) {
  fio_blake2b_s ctx = fio_blake2b_init(64, NULL, 0);
  
  /* Feed data in chunks */
  fio_blake2b_consume(&ctx, "Hello, ", 7);
  fio_blake2b_consume(&ctx, "BLAKE2!", 7);
  
  fio_u512 result = fio_blake2b_finalize(&ctx);
  /* result.u8[0..63] contains the digest */
}
```

#### HMAC Authentication

```c
void example_hmac(void) {
  const char *key = "authentication-key";
  const char *msg = "message to authenticate";

  /* BLAKE2b HMAC - 64-byte MAC */
  fio_u512 mac = fio_blake2b_hmac(key, strlen(key),
                                  msg, strlen(msg));
  /* mac.u8[0..63] contains the authentication code */

  /* BLAKE2s HMAC - 32-byte MAC */
  fio_u256 mac_s = fio_blake2s_hmac(key, strlen(key),
                                    msg, strlen(msg));
  /* mac_s.u8[0..31] contains the authentication code */
}
```

### BLAKE2b vs BLAKE2s

| Property | BLAKE2b | BLAKE2s |
|----------|---------|---------|
| Word size | 64-bit | 32-bit |
| Block size | 128 bytes | 64 bytes |
| Max digest | 64 bytes | 32 bytes |
| Max key | 64 bytes | 32 bytes |
| Rounds | 12 | 10 |
| Optimized for | 64-bit platforms | 32-bit platforms |
| Result type | `fio_u512` | `fio_u256` |

-------------------------------------------------------------------------------
