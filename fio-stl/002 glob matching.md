# Glob Matching

```c
#define FIO_GLOB_MATCH
#include "fio-stl.h"
```

Binary glob matching for wildcard patterns. Bytes in, bytes out — no UTF-8 awareness, no allocation, just `*`, `?`, `[...]`, and escapes.

### API Functions

#### `fio_glob_match`

```c
SFUNC uint8_t fio_glob_match(fio_str_info_s pattern, fio_str_info_s string);
```

Returns `1` if `string` matches `pattern`, otherwise `0`.

### Supported Patterns

#### `*`

Matches any byte sequence, including an empty one.

#### `?`

Matches any single byte.

#### `[...]`

Matches any single byte inside the brackets. Ranges work with `-`, e.g. `[a-z]`. The first character may be `]` without closing the class. Use `\` to escape `]`, `-`, and `\` itself.

#### `[!...]` or `[^...]`

Inverted character class: matches any single byte **not** in the brackets.

#### `\`

Escapes the next character so it is matched literally.

### Example

```c
#define FIO_GLOB_MATCH
#define FIO_STR
#include "fio-stl.h"

int main(void) {
  printf("%d\n", fio_glob_match(FIO_STR_INFO1("*.txt"),
                                FIO_STR_INFO1("notes.txt")));      /* 1 */
  printf("%d\n", fio_glob_match(FIO_STR_INFO1("log_????"),
                                FIO_STR_INFO1("log_2024")));       /* 1 */
  printf("%d\n", fio_glob_match(FIO_STR_INFO1("[!0-9]*"),
                                FIO_STR_INFO1("abc123")));         /* 1 */
  printf("%d\n", fio_glob_match(FIO_STR_INFO1("file\\*"),
                                FIO_STR_INFO1("file*")));          /* 1 */
  return 0;
}
```

------------------------------------------------------------
