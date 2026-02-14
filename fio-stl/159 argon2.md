## Argon2

```c
#define FIO_ARGON2
#include FIO_INCLUDE_FILE
```

By defining `FIO_ARGON2`, the Argon2 memory-hard password hashing function is defined and made available.

Argon2 is standardized in [RFC 9106](https://www.rfc-editor.org/rfc/rfc9106) and is the winner of the Password Hashing Competition (PHC). It provides resistance against GPU/ASIC attacks through configurable memory and time costs.

This implementation supports all three variants:

- **Argon2d** - data-dependent memory access (fastest, vulnerable to side-channel attacks)
- **Argon2i** - data-independent memory access (resistant to side-channel attacks)
- **Argon2id** - hybrid (recommended for password hashing)

Uses BLAKE2b as the underlying hash function. Single-threaded: the parallelism parameter affects memory layout but lanes are processed sequentially.

**Note**: when comparing Argon2 output hashes, use `fio_ct_is_eq` for constant-time comparison to avoid timing side-channel attacks.

### Argon2 Types

#### `fio_argon2_type_e`

```c
typedef enum {
  FIO_ARGON2D  = 0,  /* Data-dependent (fastest, side-channel vulnerable) */
  FIO_ARGON2I  = 1,  /* Data-independent (side-channel resistant) */
  FIO_ARGON2ID = 2,  /* Hybrid (recommended) */
} fio_argon2_type_e;
```

Selects the Argon2 variant.

- `FIO_ARGON2D` - uses data-dependent memory access patterns; fastest but vulnerable to side-channel attacks. Use for cryptocurrency proof-of-work.
- `FIO_ARGON2I` - uses data-independent memory access patterns; resistant to side-channel attacks. Use when side-channel resistance is critical.
- `FIO_ARGON2ID` - hybrid: first pass uses data-independent access, subsequent passes use data-dependent. **Recommended for password hashing.**

### Argon2 Functions

#### `fio_argon2`

```c
fio_u512 fio_argon2(fio_argon2_args_s args);
/* Named arguments using macro. */
#define fio_argon2(...) fio_argon2((fio_argon2_args_s){__VA_ARGS__})

typedef struct {
  /** The password (message P). */
  fio_buf_info_s password;
  /** The salt (nonce S). */
  fio_buf_info_s salt;
  /** Optional secret key K. */
  fio_buf_info_s secret;
  /** Optional associated data X. */
  fio_buf_info_s ad;
  /** Time cost (number of passes t, minimum 1). */
  uint32_t t_cost;
  /** Memory cost in KiB (minimum 8*parallelism). */
  uint32_t m_cost;
  /** Degree of parallelism (number of lanes, minimum 1). */
  uint32_t parallelism;
  /** Desired output (tag) length in bytes (minimum 4, default 32). */
  uint32_t outlen;
  /** Argon2 variant: FIO_ARGON2D, FIO_ARGON2I, or FIO_ARGON2ID. */
  fio_argon2_type_e type;
} fio_argon2_args_s;
```

Computes an Argon2 password hash (RFC 9106).

The function is shadowed by a macro, allowing it to accept named arguments:

```c
fio_u512 hash = fio_argon2(
    .password = FIO_BUF_INFO1("my-password"),
    .salt = FIO_BUF_INFO1("random-salt"),
    .t_cost = 3,
    .m_cost = 65536,     /* 64 MiB */
    .parallelism = 4,
    .type = FIO_ARGON2ID);
/* hash.u8[0..31] contains the 32-byte password hash */
```

**Named Arguments:**

| Argument | Type | Description |
|----------|------|-------------|
| `password` | `fio_buf_info_s` | The password to hash |
| `salt` | `fio_buf_info_s` | Salt / nonce (should be random, at least 16 bytes) |
| `secret` | `fio_buf_info_s` | Optional secret key for keyed hashing |
| `ad` | `fio_buf_info_s` | Optional associated data |
| `t_cost` | `uint32_t` | Time cost / number of passes (minimum 1) |
| `m_cost` | `uint32_t` | Memory cost in KiB (minimum 8 * parallelism) |
| `parallelism` | `uint32_t` | Number of lanes (minimum 1; processed sequentially) |
| `outlen` | `uint32_t` | Output length in bytes (minimum 4, default 32) |
| `type` | `fio_argon2_type_e` | Variant: `FIO_ARGON2D`, `FIO_ARGON2I`, or `FIO_ARGON2ID` |

**Returns:** a `fio_u512` containing the password hash. Only the first `outlen` bytes are valid (default 32). For output lengths greater than 64 bytes, use `fio_argon2_hash` instead.

#### `fio_argon2_hash`

```c
int fio_argon2_hash(void *out, fio_argon2_args_s args);
/* Named arguments using macro. */
#define fio_argon2_hash(out, ...) fio_argon2_hash(out, (fio_argon2_args_s){__VA_ARGS__})
```

Computes an Argon2 password hash into a caller-provided buffer.

Supports arbitrary output lengths (minimum 4 bytes). Use this instead of `fio_argon2` when the desired output length exceeds 64 bytes.

The function is shadowed by a macro, allowing it to accept named arguments:

```c
uint8_t hash[128];
int r = fio_argon2_hash(hash,
    .password = FIO_BUF_INFO1("my-password"),
    .salt = FIO_BUF_INFO1("random-salt"),
    .t_cost = 3,
    .m_cost = 65536,
    .parallelism = 4,
    .outlen = 128,
    .type = FIO_ARGON2ID);
```

**Parameters:**
- `out` - destination buffer (must have capacity for at least `outlen` bytes)
- Named arguments are the same as `fio_argon2` (see table above)

**Returns:** 0 on success, -1 on error.

### Argon2 Examples

#### Password Hashing (Recommended Settings)

```c
#define FIO_ARGON2
#include FIO_INCLUDE_FILE

void example_password_hash(void) {
  /* OWASP recommended minimum: Argon2id, t=3, m=64MiB, p=4 */
  fio_u512 hash = fio_argon2(
      .password = FIO_BUF_INFO1("user-password"),
      .salt = FIO_BUF_INFO1("unique-random-salt"),
      .t_cost = 3,
      .m_cost = 65536,     /* 64 MiB */
      .parallelism = 4,
      .type = FIO_ARGON2ID);

  /* Verify: recompute and compare in constant time */
  fio_u512 verify = fio_argon2(
      .password = FIO_BUF_INFO1("user-password"),
      .salt = FIO_BUF_INFO1("unique-random-salt"),
      .t_cost = 3,
      .m_cost = 65536,
      .parallelism = 4,
      .type = FIO_ARGON2ID);

  if (fio_ct_is_eq(hash.u8, verify.u8, 32))
    printf("Password matches!\n");
}
```

-------------------------------------------------------------------------------
