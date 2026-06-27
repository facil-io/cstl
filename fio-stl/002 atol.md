# String / Number Conversion

```c
#define FIO_ATOL
#include "fio-stl.h"
```

String-to-number and number-to-string helpers. These are the grunts that parse integers, floats, hex, binary, and arbitrary bases, then turn them back into text. Fast, greedy, and mostly guard-less — give them a valid buffer and a terminating character.

**Note:** functions that write to a buffer also write a NUL terminator. `fio_atol*` functions assume the buffer ends with an invalid character (such as NUL) and that allocations are aligned enough for multi-byte reads.

### Configuration Macros

#### `FIO_ATOL_ALLOW_UNDERSCORE_DIVIDER`

```c
#define FIO_ATOL_ALLOW_UNDERSCORE_DIVIDER 1
```

When `1` (default), underscores act as digit separators: `1_000_000` parses as `1000000`. Set to `0` to disable.

### Types

#### `fio_aton_s`

```c
typedef struct {
  union {
    int64_t i;
    double f;
    uint64_t u;
  };
  int is_float;
  int err;
} fio_aton_s;
```

Result container for `fio_aton`. Read the union member that matches `is_float`, and check `err` for overflow or parse failures.

### Universal Parsing

#### `fio_aton`

```c
FIO_SFUNC fio_aton_s fio_aton(char **pstr);
```

Auto-detects integers and floats. Skips leading whitespace, recognizes `0x` / `0b` / octal prefixes, and accepts `inf`, `infinity`, and `nan`. Updates `*pstr` to the first unconverted character. Sets `.err` on overflow or bad format.

**Note:** not an exact `strtod` replacement; rounding differences are possible.

### Signed Conversion

#### `fio_atol`

```c
SFUNC int64_t fio_atol(char **pstr);
```

Parses a signed `int64_t`. Accepts base 10, octal (`0...`), hex (`0x...` or `x...`), and binary (`0b...` or `b...`). Updates `*pstr` past the number.

#### `fio_atof`

```c
SFUNC double fio_atof(char **pstr);
```

Parses a double. Wraps `strtod` for most inputs. The source also attempts to accept a raw `0b...` binary bit-pattern, but the detection condition looks fragile.

#### `fio_ftoa`

```c
SFUNC size_t fio_ftoa(char *dest, double num, uint8_t base);
```

Writes `num` to `dest` in `base` (2, 10, or 16; unsupported bases silently fall back to 10). No prefixes are added. Returns bytes written excluding NUL.

**Note:** provide at least 130 bytes for base 2. Special values `inf` and `nan` produce `"Infinity"` / `"NaN"`.

#### `fio_ltoa`

```c
SFUNC size_t fio_ltoa(char *dest, int64_t num, uint8_t base);
```

Writes `num` to `dest` in `base` (2, 8, 10, 16, or any base up to 36). Adds `0x`, `0b`, or `0` prefixes for the built-in bases. Returns bytes written excluding NUL. If `dest` is `NULL`, writes to an internal scratch buffer and still returns the length. Logs an error and returns `0` for unsupported bases.

**Note:** provide at least 68 bytes for base 2.

#### `fio_ltoa10`

```c
FIO_IFUNC void fio_ltoa10(char *dest, int64_t i, size_t digits);
```

Writes a signed base-10 number using exactly `digits` bytes plus NUL. Use `fio_digits10()` to compute `digits`.

#### `fio_atol10`

```c
SFUNC int64_t fio_atol10(char **pstr);
```

Reads a signed base-10 number.

### Unsigned Conversion

#### `fio_atol8u`

```c
SFUNC uint64_t fio_atol8u(char **pstr);
```

Reads an unsigned octal number. May overflow the buffer if no terminator is present.

#### `fio_atol10u`

```c
SFUNC uint64_t fio_atol10u(char **pstr);
```

Reads an unsigned base-10 number.

#### `fio_atol16u`

```c
SFUNC uint64_t fio_atol16u(char **pstr);
```

Reads an unsigned hex number, with optional `0x` prefix.

#### `fio_atol_bin`

```c
SFUNC uint64_t fio_atol_bin(char **pstr);
```

Reads an unsigned binary number, with optional `0b` prefix.

#### `fio_atol_xbase`

```c
SFUNC uint64_t fio_atol_xbase(char **pstr, size_t base);
```

Reads an unsigned number in any base up to 36.

#### `fio_ltoa8u`

```c
FIO_IFUNC void fio_ltoa8u(char *dest, uint64_t i, size_t digits);
```

Writes an unsigned octal number using `digits` bytes plus NUL.

#### `fio_ltoa10u`

```c
FIO_IFUNC void fio_ltoa10u(char *dest, uint64_t i, size_t digits);
```

Writes an unsigned base-10 number using `digits` bytes plus NUL.

#### `fio_ltoa16u`

```c
FIO_IFUNC void fio_ltoa16u(char *dest, uint64_t i, size_t digits);
```

Writes an unsigned hex number using `digits` bytes plus NUL. `digits` is rounded up to an even number.

#### `fio_ltoa_bin`

```c
FIO_IFUNC void fio_ltoa_bin(char *dest, uint64_t i, size_t digits);
```

Writes an unsigned binary number using `digits` bytes plus NUL.

#### `fio_ltoa_xbase`

```c
FIO_IFUNC void fio_ltoa_xbase(char *dest,
                              uint64_t i,
                              size_t digits,
                              size_t base);
```

Writes an unsigned number in `base` (up to 36) using `digits` bytes plus NUL.

### Helpers

#### `fio_c2i`

```c
IFUNC uint8_t fio_c2i(unsigned char c);
```

Maps a character to its numeric value (`0-9` → 0-9, `A-Z`/`a-z` → 10-35). Returns `255` for out-of-range characters.

#### `fio_i2c`

```c
IFUNC uint8_t fio_i2c(unsigned char i);
```

Maps a numeric value `0-35` to a character (`0-9`, `A-Z`). Out-of-range values above 35 produce undefined behavior; accepts values up to 63 by masking.

#### `fio_digits10`

```c
FIO_IFUNC size_t fio_digits10(int64_t i);
```

Returns the number of base-10 digits needed for `i`, including the sign.

#### `fio_digits10u`

```c
FIO_SFUNC size_t fio_digits10u(uint64_t i);
```

Returns the number of base-10 digits needed for an unsigned number.

#### `fio_digits8u`

```c
FIO_SFUNC size_t fio_digits8u(uint64_t i);
```

Returns the number of base-8 digits needed for an unsigned number.

#### `fio_digits16u`

```c
FIO_SFUNC size_t fio_digits16u(uint64_t i);
```

Returns the number of base-16 digits needed for an unsigned number, always an even count (2, 4, 6, ... 16).

#### `fio_digits_bin`

```c
FIO_SFUNC size_t fio_digits_bin(uint64_t i);
```

Returns the number of base-2 digits needed for an unsigned number, rounded up to an even count.

#### `fio_digits_xbase`

```c
FIO_SFUNC size_t fio_digits_xbase(uint64_t i, size_t base);
```

Returns the number of digits needed for an unsigned number in `base` (must be < 65).

#### `fio_u2i_limit`

```c
FIO_IFUNC int64_t fio_u2i_limit(uint64_t val, size_t invert);
```

Converts unsigned `val` to signed with overflow protection. If `invert` is zero, clamps to `INT64_MAX` and sets `errno = E2BIG` on overflow. If `invert` is non-zero, produces the negative value and clamps to `INT64_MIN`.

### IEEE 754 Helpers

#### `fio_i2d`

```c
FIO_IFUNC double fio_i2d(int64_t mant, int64_t exponent_in_base_2);
```

Converts a signed 64-bit mantissa and base-2 exponent to a `double`.

#### `fio_u2d`

```c
FIO_IFUNC double fio_u2d(uint64_t mant, int64_t exponent_in_base_2);
```

Converts an unsigned 64-bit mantissa and base-2 exponent to a `double`.

### Big Number Hex Conversion

#### `fio_u128_hex_read` / `fio_u256_hex_read` / `fio_u512_hex_read`

```c
SFUNC fio_u128 fio_u128_hex_read(char **pstr);
SFUNC fio_u256 fio_u256_hex_read(char **pstr);
SFUNC fio_u512 fio_u512_hex_read(char **pstr);
```

Reads a hex string and initializes the corresponding wide integer. Updates `*pstr` past the consumed input.

#### `fio_u1024_hex_read` / `fio_u2048_hex_read` / `fio_u4096_hex_read`

```c
SFUNC fio_u1024 fio_u1024_hex_read(char **pstr);
SFUNC fio_u2048 fio_u2048_hex_read(char **pstr);
SFUNC fio_u4096 fio_u4096_hex_read(char **pstr);
```

Same as above for 1024-, 2048-, and 4096-bit integers.

#### `fio_u128_hex_write` / `fio_u256_hex_write` / `fio_u512_hex_write`

```c
SFUNC size_t fio_u128_hex_write(char *dest, const fio_u128 *u);
SFUNC size_t fio_u256_hex_write(char *dest, const fio_u256 *u);
SFUNC size_t fio_u512_hex_write(char *dest, const fio_u512 *u);
```

Writes a wide integer as a hex string to `dest`. Returns bytes written excluding NUL.

#### `fio_u1024_hex_write` / `fio_u2048_hex_write` / `fio_u4096_hex_write`

```c
SFUNC size_t fio_u1024_hex_write(char *dest, const fio_u1024 *u);
SFUNC size_t fio_u2048_hex_write(char *dest, const fio_u2048 *u);
SFUNC size_t fio_u4096_hex_write(char *dest, const fio_u4096 *u);
```

Same as above for 1024-, 2048-, and 4096-bit integers.

### Example

```c
#define FIO_ATOL
#include "fio-stl.h"

int main(void) {
  char *p = "0x1F 0b1010 42";
  char buf[80];

  printf("hex: %lld\n", (long long)fio_atol(&p));
  ++p; /* skip space */
  printf("bin: %lld\n", (long long)fio_atol(&p));
  ++p;
  printf("dec: %lld\n", (long long)fio_atol(&p));

  fio_ltoa(buf, -255, 16);
  printf("back to hex: %s\n", buf);
  return 0;
}
```

------------------------------------------------------------
