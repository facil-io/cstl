# Argon2

```c
#define FIO_ARGON2
#include "fio-stl.h"
```

Argon2 is a memory-hard password hashing function from RFC 9106. It uses BLAKE2b underneath and lets you tune time, memory, and lane count so attackers have to pay real rent.

This implementation supports Argon2d, Argon2i, and Argon2id. It is single-threaded: `parallelism` affects the Argon2 lane layout, but lanes are processed sequentially.

When comparing Argon2 outputs, use `fio_ct_is_eq` or another constant-time comparison.

## Types

### `fio_argon2_type_e`

```c
typedef enum {
  FIO_ARGON2D = 0,
  FIO_ARGON2I = 1,
  FIO_ARGON2ID = 2,
} fio_argon2_type_e;
```

- `FIO_ARGON2D`: data-dependent access, faster, weaker against side-channel observers.
- `FIO_ARGON2I`: data-independent access.
- `FIO_ARGON2ID`: hybrid mode, commonly recommended for password hashing.

### `fio_argon2_args_s`

```c
typedef struct {
  fio_buf_info_s password;
  fio_buf_info_s salt;
  fio_buf_info_s secret;
  fio_buf_info_s ad;
  uint32_t t_cost;
  uint32_t m_cost;
  uint32_t parallelism;
  uint32_t outlen;
  fio_argon2_type_e type;
} fio_argon2_args_s;
```

Arguments:

| Field | Meaning |
| --- | --- |
| `password` | Password / message `P`. |
| `salt` | Salt / nonce `S`; use a unique random salt. |
| `secret` | Optional secret key `K`. |
| `ad` | Optional associated data `X`. |
| `t_cost` | Pass count, minimum `1`. |
| `m_cost` | Memory cost in KiB, minimum `8 * parallelism`. |
| `parallelism` | Lane count, minimum `1`; processed sequentially here. |
| `outlen` | Output length in bytes, minimum `4`, default `32`. |
| `type` | `FIO_ARGON2D`, `FIO_ARGON2I`, or `FIO_ARGON2ID`. |

## API

### `fio_argon2`

```c
SFUNC fio_u512 fio_argon2(fio_argon2_args_s args);
#define fio_argon2(...) fio_argon2((fio_argon2_args_s){__VA_ARGS__})
```

Computes an Argon2 hash and returns up to 64 bytes in `fio_u512`. Use the first `outlen` bytes; if `outlen` is `0`, the default is 32 bytes.

For outputs longer than 64 bytes, use `fio_argon2_hash`.

### `fio_argon2_hash`

```c
SFUNC int fio_argon2_hash(void *out, fio_argon2_args_s args);
#define fio_argon2_hash(out, ...)                                              \
  fio_argon2_hash(out, (fio_argon2_args_s){__VA_ARGS__})
```

Writes the Argon2 output into a caller-provided buffer. Supports arbitrary output lengths of at least 4 bytes.

Returns `0` on success, `-1` on error.

## Example

```c
#define FIO_ARGON2
#include "fio-stl.h"

int check_password(const char *password,
                   size_t password_len,
                   const void *salt,
                   size_t salt_len,
                   const uint8_t expected[32]) {
  fio_u512 hash = fio_argon2(
      .password = FIO_BUF_INFO2((void *)password, password_len),
      .salt = FIO_BUF_INFO2((void *)salt, salt_len),
      .t_cost = 3,
      .m_cost = 65536,     /* 64 MiB */
      .parallelism = 4,
      .outlen = 32,
      .type = FIO_ARGON2ID);

  return fio_ct_is_eq(hash.u8, expected, 32) ? 0 : -1;
}
```

## Practical Notes

- Store the salt and cost settings with the password hash. Future you will not remember them. Future you never does.
- Pick `FIO_ARGON2ID` for password hashing unless you have a specific reason not to.
- Raise `m_cost` and `t_cost` until the slow path is acceptable for your login flow.
- Do not compare hashes with `memcmp`; use constant-time comparison.

------------------------------------------------------------
