# ML Entity Decoding

```c
#define FIO_ENTITY
#include "fio-stl.h"
```

Decodes a single Markup Language entity (`&name;`, `&#digits;`, `&#xhex;`) into UTF-8. Handy when you need to unescape one entity at a time, for example while scanning HTML or XML text.

### API Functions

#### `fio_entity`

```c
SFUNC size_t fio_entity(char *dest, const char *src, size_t len);
```

Decodes the entity starting at `src` (length `len`). Writes the UTF-8 result to `dest`, which must be at least 8 bytes. A NUL terminator is written only if the result is shorter than 8 bytes.

**Returns:** the number of bytes written (excluding NUL), or `0` if `src` does not start with a valid entity.

Supported forms:

- Named: `&name;` — case-insensitive, ~40 common entities such as `&lt;`, `&amp;`, `&copy;`, `&mdash;`.
- Decimal: `&#digits;`
- Hex: `&#xhex;` or `&#Xhex;`

`&#0;` is replaced with the Unicode replacement character (`U+FFFD`). Code points above `0x10FFFF` are rejected.

### Example

```c
#define FIO_ENTITY
#include "fio-stl.h"

int main(void) {
  const char *s = "&copy; 2026 &#x1F600;";
  char buf[8];
  size_t n = fio_entity(buf, s, strlen(s));
  printf("decoded: %.*s\n", (int)n, buf);
  return 0;
}
```

------------------------------------------------------------
