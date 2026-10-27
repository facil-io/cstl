# BLAKE2

```c
#define FIO_BLAKE2
#include "fio-stl.h"
```

BLAKE2b (64-bit, up to 64-byte digest) and BLAKE2s (32-bit, up to 32-byte digest). Both are fast, modern cryptographic hashes with built-in keyed hashing, standard HMAC wrappers, and a streaming init/consume/finalize API.

### Types

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

Streaming BLAKE2b context. Treat it as opaque and initialize with `fio_blake2b_init`.

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

Streaming BLAKE2s context. Treat it as opaque and initialize with `fio_blake2s_init`.

### BLAKE2b Functions

#### `fio_blake2b`

```c
FIO_IFUNC fio_u512 fio_blake2b(const void *data, uint64_t len);
```

One-shot BLAKE2b with the full 64-byte digest.

**Parameters:**
- `data` — bytes to hash.
- `len` — number of bytes.

**Returns:** the 64-byte digest as `fio_u512`.

**Note:** signature matches `fio_sha512`, so the two are interchangeable as `fio_u512 (*)(const void *, uint64_t)`.

#### `fio_blake2b_hash`

```c
SFUNC void fio_blake2b_hash(void *restrict out,
                            size_t outlen,
                            const void *restrict data,
                            size_t len,
                            const void *restrict key,
                            size_t keylen);
```

Flexible-output BLAKE2b with optional keyed hashing.

**Parameters:**
- `out` — destination buffer (at least `outlen` bytes).
- `outlen` — digest length (1–64; defaults to 64 if 0).
- `data` — bytes to hash.
- `len` — data length.
- `key` — optional secret key (NULL for unkeyed; a NULL `key` ignores `keylen`).
- `keylen` — key length. 0–64 is standard keyed BLAKE2b. Longer keys are never truncated: the key is replaced by `BLAKE2b-512(key)` (64 bytes), like HMAC does. This is **non-standard** (other BLAKE2 libraries reject such keys), so use keys of at most 64 bytes for interoperable output.

**Note:** keyed BLAKE2b is a built-in MAC mode; for RFC-2104 HMAC, use `fio_blake2b_hmac`.

#### `fio_blake2b_hmac`

```c
SFUNC fio_u512 fio_blake2b_hmac(const void *key,
                                uint64_t key_len,
                                const void *msg,
                                uint64_t msg_len);
```

Standard HMAC-BLAKE2b with a 64-byte digest.

**Parameters:**
- `key` — secret key.
- `key_len` — key length.
- `msg` — message to authenticate.
- `msg_len` — message length.

**Returns:** 64-byte authentication code as `fio_u512`.

#### `fio_blake2b_init`

```c
SFUNC fio_blake2b_s fio_blake2b_init(size_t outlen,
                                     const void *key,
                                     size_t keylen);
```

Initializes a BLAKE2b streaming context.

**Parameters:**
- `outlen` — desired digest length (1–64; defaults to 64 if 0).
- `key` — optional secret key (NULL for unkeyed; a NULL `key` ignores `keylen`, producing the standard unkeyed digest).
- `keylen` — key length; keys over 64 bytes are pre-hashed (non-standard), as in `fio_blake2b_hash`.

The context is built directly in the returned object (no intermediate key copy with compilers that apply return-value optimization, e.g. clang at any level, GCC at `-O1`+). Assign the result in its declaration (`fio_blake2b_s h = fio_blake2b_init(...)`) rather than to an existing variable, so no temporary key copy is created. To avoid key copies regardless of compiler, use `fio_blake2b_init_at`. The one-shot `fio_blake2b_hash` never copies the key.

#### `fio_blake2b_init_at`

```c
SFUNC void fio_blake2b_init_at(fio_blake2b_s *h,
                               size_t outlen,
                               const void *key,
                               size_t keylen);
```

Initializes `h` in place, with the same arguments and results as `fio_blake2b_init`. The padded key is written only into `h` (and wiped by `fio_blake2b_finalize`).

#### `fio_blake2b_consume`

```c
SFUNC void fio_blake2b_consume(fio_blake2b_s *restrict h,
                               const void *restrict data,
                               size_t len);
```

Feeds more data into a BLAKE2b streaming context. May be called repeatedly.

#### `fio_blake2b_finalize`

```c
SFUNC fio_u512 fio_blake2b_finalize(fio_blake2b_s *h);
```

Finalizes the stream and returns the digest. The context (including any key material) is securely wiped; re-initialize it before reuse.

**Returns:** digest as `fio_u512`. Only the first `outlen` bytes are valid.

### BLAKE2s Functions

#### `fio_blake2s`

```c
FIO_IFUNC fio_u256 fio_blake2s(const void *data, uint64_t len);
```

One-shot BLAKE2s with the full 32-byte digest.

#### `fio_blake2s_hash`

```c
SFUNC void fio_blake2s_hash(void *restrict out,
                            size_t outlen,
                            const void *restrict data,
                            size_t len,
                            const void *restrict key,
                            size_t keylen);
```

Flexible-output BLAKE2s with optional keyed hashing. `outlen` is 1–32 (defaults to 32 if 0). `keylen` 0–32 is standard keyed BLAKE2s; longer keys are replaced by `BLAKE2s-256(key)` (non-standard, HMAC-like, never truncated). A NULL `key` is unkeyed and ignores `keylen`.

#### `fio_blake2s_hmac`

```c
SFUNC fio_u256 fio_blake2s_hmac(const void *key,
                                uint64_t key_len,
                                const void *msg,
                                uint64_t msg_len);
```

Standard HMAC-BLAKE2s with a 32-byte digest.

#### `fio_blake2s_init`

```c
SFUNC fio_blake2s_s fio_blake2s_init(size_t outlen,
                                     const void *key,
                                     size_t keylen);
```

Initializes a BLAKE2s streaming context. `outlen` is 1–32 (defaults to 32 if 0). Key handling matches `fio_blake2s_hash` (NULL key is unkeyed; keys over 32 bytes are pre-hashed) and the context is built in place in the returned object, as with `fio_blake2b_init`.

#### `fio_blake2s_init_at`

```c
SFUNC void fio_blake2s_init_at(fio_blake2s_s *h,
                               size_t outlen,
                               const void *key,
                               size_t keylen);
```

Initializes `h` in place, with the same arguments and results as `fio_blake2s_init`, so no key copy is left elsewhere on the stack.

#### `fio_blake2s_consume`

```c
SFUNC void fio_blake2s_consume(fio_blake2s_s *restrict h,
                               const void *restrict data,
                               size_t len);
```

Feeds more data into a BLAKE2s streaming context.

#### `fio_blake2s_finalize`

```c
SFUNC fio_u256 fio_blake2s_finalize(fio_blake2s_s *h);
```

Finalizes the stream and returns the digest. The context (including any key material) is securely wiped; re-initialize it before reuse.

### Examples

```c
#define FIO_BLAKE2
#include "fio-stl.h"

void examples(void) {
  const char *msg = "Hello, BLAKE2!";

  /* One-shot */
  fio_u512 b2b = fio_blake2b(msg, strlen(msg));
  fio_u256 b2s = fio_blake2s(msg, strlen(msg));

  /* Flexible output with key */
  uint8_t digest[16];
  fio_blake2b_hash(digest, 16, msg, strlen(msg), "key", 3);

  /* Streaming */
  fio_blake2b_s ctx = fio_blake2b_init(64, NULL, 0);
  fio_blake2b_consume(&ctx, "Hello, ", 7);
  fio_blake2b_consume(&ctx, "BLAKE2!", 7);
  fio_u512 result = fio_blake2b_finalize(&ctx);

  /* HMAC */
  fio_u512 mac = fio_blake2b_hmac("key", 3, msg, strlen(msg));
}
```

### BLAKE2b vs BLAKE2s

| Property | BLAKE2b | BLAKE2s |
|---|---|---|
| Word size | 64-bit | 32-bit |
| Block size | 128 bytes | 64 bytes |
| Max digest | 64 bytes | 32 bytes |
| Max standard key | 64 bytes | 32 bytes (longer keys are pre-hashed) |
| Rounds | 12 | 10 |
| Result type | `fio_u512` | `fio_u256` |

------------------------------------------------------------
