## Secrets

```c
#define FIO_SECRET
#include FIO_INCLUDE_FILE
```

By defining the `FIO_SECRET`, a small secret helper API will be provided. This requires the `FIO_SHA2` and `FIO_ATOL` modules.

### Keeping Secrets

Secrets should be kept secret. 

Secrets should **not** be kept in source files, which too often end up end up exposed.

Additionally, secrets should **not** be used directly. It is better to use a hashed value of the secret, possible with some time based salt or spice.

This way, if somehow information leaks regarding the secret, what is exposed is actually the hashed value and not the secret itself.

For this reason, the most common place to place a secret is as a hashed value in the OS environment (often as a Hex encoded String).

The following helper functions are defined:


#### `fio_secret_is_random`

```c
bool fio_secret_is_random(void);
```

Returns true if the secret was randomly generated.

#### `fio_secret`

```c
fio_u512 fio_secret(void);
```

Gets the SHA512 of a (possibly shared) secret.

#### `fio_secret_set`

```c
void fio_secret_set(char *str, size_t len, bool is_random);
```

Sets a (possibly shared) secret and stores its SHA512 hash.

-------------------------------------------------------------------------------
