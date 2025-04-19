## Secrets

```c
#define FIO_SECRET
#include FIO_INCLUDE_FILE
```

By defining the `FIO_SECRET`, a small secret helper API will be provided. This requires the `FIO_SHA2` and `FIO_ATOL` modules.

### Keeping Secrets

Secrets should be kept secret. 

Secrets should **not** be kept in source files, which too often end up end up exposed.

Secrets should **not** be logged (such as in case of a crash or a core dump).

Additionally, secrets should **not** be used directly if possible. It is better to use a hashed value of the secret, possible with some time based salt or spice. This way, if somehow information leaks regarding the secret, what is exposed is actually the hashed value and not the secret itself.

For this reason, the most common place to place a secret is as a hashed value in the OS environment (often as a Hex encoded String).

**Note**: some secrets, such as TLS certificates, are often stored as system files somewhere separate from the source code.

To help with managing a program wide secret, the following helper functions are defined:


#### `fio_secret_set_at`

```c
void fio_secret_set_at(fio_u512 *secret, char *str, size_t len);
```

Sets a (possibly shared) secret and stores its (masked) SHA512 hash in `secret`.

**Note**: the SHA512 hash in `secret` is masked before it is stored, so that the final secret isn't logged in case of a core dump.

#### `fio_secret_at`

```c
fio_u512 fio_secret_at(fio_u512 *secret);
```

Gets the SHA512 of a (possibly shared) masked secret stored in `secret`.

Please store the returned value on the stack or not at all. The secret is stored masked in memory and unmasked copies should be temporary with short life-spans.

#### `fio_secret`

```c
fio_u512 fio_secret(void);
```

Returns the SHA512 of a (possibly shared) secret.

Unless updated using `fio_secret_set`, this is either a random secret or the one derived from the `SECRET` environment variable.

Please store the returned value on the stack or not at all. The secret is stored masked in memory and unmasked copies should be temporary with short life-spans.

#### `fio_secret_set`

```c
void fio_secret_set(char *secret, size_t len, bool is_random);
```

Sets a (possibly shared) secret and stores its SHA512 hash.

If `secret` is Hex encoded, it will be decoded before it is hashed and white spaces will be ignored.

**Note**: the SHA512 hash is masked before it is stored, so that the final secret isn't logged in case of a core dump.


#### `fio_secret_is_random`

```c
bool fio_secret_is_random(void);
```

Returns true if the secret was randomly generated.


-------------------------------------------------------------------------------
