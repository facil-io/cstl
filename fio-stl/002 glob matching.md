## Glob Matching

```c
#define FIO_GLOB_MATCH
#include "fio-stl.h"
```

By defining the macro `FIO_GLOB_MATCH`, the following glob pattern matching function is defined. This provides a binary glob matching helper useful for filtering strings against wildcard patterns.

#### `fio_glob_match`

```c
uint8_t fio_glob_match(fio_str_info_s pattern, fio_str_info_s string);
```

A **binary** glob matching helper that tests if `string` matches the glob `pattern`.

**Parameters:**

- `pattern` - the glob pattern to match against
- `string` - the string to test

**Returns:** `1` on a match, `0` otherwise.

**Note**: this function operates on raw bytes and does **not** support UTF-8 multi-byte characters for single-character matching (`?` and `[...]`).

### Supported Patterns

The following glob patterns are recognized:

#### Wildcard `*`

Matches any string, including an empty string.

```c
/* The following patterns will match against the string "String": */
fio_glob_match(FIO_STR_INFO1("*"), FIO_STR_INFO1("String"));        /* matches */
fio_glob_match(FIO_STR_INFO1("*String*"), FIO_STR_INFO1("String")); /* matches */
fio_glob_match(FIO_STR_INFO1("S*ing"), FIO_STR_INFO1("String"));    /* matches */
```

#### Single Character `?`

Matches any single **byte** (does NOT support UTF-8 characters).

```c
/* The following patterns will match against the string "String": */
fio_glob_match(FIO_STR_INFO1("?tring"), FIO_STR_INFO1("String")); /* matches */
fio_glob_match(FIO_STR_INFO1("Strin?"), FIO_STR_INFO1("String")); /* matches */
fio_glob_match(FIO_STR_INFO1("St?ing"), FIO_STR_INFO1("String")); /* matches */
```

#### Negated Character Class `[!...]` or `[^...]`

Matches any **byte** that is **not** within the brackets (does **not** support UTF-8 characters).

Byte ranges are supported using `-` (e.g., `[!0-9]` matches any non-digit).

Use the backslash (`\`) to escape the special `]`, `-` and `\` characters when they are part of the list.

```c
/* The following patterns will match against the string "String": */
fio_glob_match(FIO_STR_INFO1("[!a-z]tring"), FIO_STR_INFO1("String")); /* matches */
fio_glob_match(FIO_STR_INFO1("[^a-z]tring"), FIO_STR_INFO1("String")); /* matches */
fio_glob_match(FIO_STR_INFO1("[^F]tring"), FIO_STR_INFO1("String"));   /* matches */
```

#### Character Class `[...]`

Matches any **byte** that **is** within the brackets (does **not** support UTF-8 characters).

Byte ranges are supported using `-` (e.g., `[a-z]` matches any lowercase letter).

Use the backslash (`\`) to escape the special `]`, `-` and `\` characters when they are part of the list.

```c
/* The following patterns will match against the string "String": */
fio_glob_match(FIO_STR_INFO1("[A-Z]tring"), FIO_STR_INFO1("String")); /* matches */
fio_glob_match(FIO_STR_INFO1("[sS]tring"), FIO_STR_INFO1("String"));  /* matches */
```

#### Escape Character `\`

The backslash can be used to escape special characters (`*`, `?`, `[`, `\`) so they are matched literally.

```c
/* Match a literal asterisk */
fio_glob_match(FIO_STR_INFO1("file\\*"), FIO_STR_INFO1("file*")); /* matches */
```

### Example

```c
#define FIO_GLOB_MATCH
#define FIO_STR
#include "fio-stl.h"

int main(void) {
  fio_str_info_s pattern = FIO_STR_INFO1("*.txt");
  fio_str_info_s filename = FIO_STR_INFO1("document.txt");

  if (fio_glob_match(pattern, filename)) {
    printf("File matches pattern!\n");
  } else {
    printf("No match.\n");
  }

  /* More examples */
  fio_glob_match(FIO_STR_INFO1("test_*"), FIO_STR_INFO1("test_file"));   /* 1 */
  fio_glob_match(FIO_STR_INFO1("data[0-9]"), FIO_STR_INFO1("data5"));    /* 1 */
  fio_glob_match(FIO_STR_INFO1("log_????"), FIO_STR_INFO1("log_2024"));  /* 1 */
  fio_glob_match(FIO_STR_INFO1("[A-Z]*"), FIO_STR_INFO1("Hello"));       /* 1 */
  fio_glob_match(FIO_STR_INFO1("[!0-9]*"), FIO_STR_INFO1("abc"));        /* 1 */

  return 0;
}
```

-------------------------------------------------------------------------------
