# Secrets

```c
#define FIO_SECRET
#include "fio-stl.h"
```

A small program-wide secret helper. It reads an optional `SECRET` environment variable at startup, hashes it with SHA-512, and stores the result masked in memory so the raw secret does not show up in core dumps. If no secret is provided, a random one is generated.

This is **not** a key-management vault — it is a convenience for deriving a stable program-wide token from something outside the source tree.

### Environment Variables

At startup (via `FIO_CONSTRUCTOR`), the module checks:

- `SECRET` — the secret value, plain text or Hex encoded.
- `SECRET_LENGTH` — optional byte length (`1..strlen(SECRET)`) that truncates `SECRET`; if omitted, `strlen` is used. Invalid values (non-numeric, negative, zero or longer than `SECRET`) are ignored with a warning.

If `SECRET` is unset, empty or whitespace only, a random 512-bit secret is generated.

### API Functions

#### `fio_secret_is_random`

```c
SFUNC bool fio_secret_is_random(void);
```

Returns `true` if the global secret was randomly generated (rather than loaded from the environment).

#### `fio_secret`

```c
SFUNC fio_u512 fio_secret(void);
```

Returns the global secret as a masked SHA-512 hash. The value is unmasked on return; keep the result on the stack and erase it promptly.

**Returns:** the SHA-512 hash of the program secret, unmasked.

#### `fio_secret_set`

```c
SFUNC void fio_secret_set(char *str, size_t len, bool is_random);
```

Sets the global secret from `str` and stores its masked SHA-512 hash.

**Parameters:**
- `str` — secret bytes; may be plain text or Hex encoded. Whitespace between Hex byte pairs is ignored during Hex decoding.
- `len` — length of `str` in bytes.
- `is_random` — set `true` if the secret is randomly generated.

If `str` is `NULL`, `len` is `0` or `str` contains only whitespace, a random secret is generated and `is_random` is forced to `true`.

Setters are meant for initialization (before threads are started); they are not synchronized with concurrent readers.

#### `fio_secret_set_at`

```c
SFUNC void fio_secret_set_at(fio_u512 *secret, char *str, size_t len);
```

Hashes `str` with SHA-512 and stores the masked result in `secret`.

**Parameters:**
- `secret` — destination hash. If `NULL`, the function returns silently.
- `str` — secret bytes; Hex decoding and whitespace handling apply as in `fio_secret_set`.
- `len` — length of `str`.

If `str` is `NULL`, `len` is `0` or `str` contains only whitespace, random bytes are hashed instead.

All values share one process-lifetime mask, so values stored earlier remain valid after later `fio_secret_set` / `fio_secret_set_at` calls.

#### `fio_secret_at`

```c
SFUNC fio_u512 fio_secret_at(fio_u512 *secret);
```

Unmasks a secret hash stored by `fio_secret_set_at`.

**Parameters:**
- `secret` — pointer to a masked `fio_u512`.

**Returns:** the unmasked SHA-512 hash (all zeros if `secret` is `NULL`).

### Example

```c
#define FIO_SECRET
#include "fio-stl.h"

int main(void) {
  /* Set a custom secret and read it back. */
  fio_secret_set("hunter2", 7, 0);
  fio_u512 s = fio_secret();
  /* s now holds SHA-512("hunter2"), masked while in global storage. */
  fio_secure_zero(&s, sizeof(s));
  return 0;
}
```

------------------------------------------------------------
