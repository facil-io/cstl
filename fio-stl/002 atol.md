## String / Number Conversion

```c
#define FIO_ATOL
#include "fio-stl.h"
```

If the `FIO_ATOL` macro is defined, the following functions will be defined for converting between strings and numbers.

**Note**: all functions that write to a buffer also write a `NUL` terminator byte.

### Configuration Macros

#### `FIO_ATOL_ALLOW_UNDERSCORE_DIVIDER`

```c
#define FIO_ATOL_ALLOW_UNDERSCORE_DIVIDER 1
```

When set to `1` (default), allows underscores (`_`) to be used as digit separators when parsing numbers. For example, `1_000_000` would be parsed as `1000000`.

Set to `0` to disable this feature.

### Universal Number Parsing

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

Result type for `fio_aton`. Contains the parsed number value and metadata about the parsing result.

**Members:**
- `i` - The parsed value as a signed 64-bit integer
- `f` - The parsed value as a double-precision float
- `u` - The parsed value as an unsigned 64-bit integer
- `is_float` - Non-zero if the parsed value is a floating-point number
- `err` - Non-zero if a parsing or overflow error occurred

#### `fio_aton`

```c
fio_aton_s fio_aton(char **pstr);
```

Converts a string to a number - either an integer or a float (double).

- Skips white space at the beginning of the string
- Auto detects binary and hex formats when prefix is provided (`0x` / `0b`)
- Auto detects octal when number starts with zero
- Auto detects the strings `"inf"`, `"infinity"` and `"nan"` as float values
- The number's format and type are returned in the return type
- If a numerical overflow or format error occurred, the `.err` flag is set

**Note**: rounding errors may occur, as this is not an exact `strtod` match.

### Signed Number / String Conversion

The most common use of number to string conversion (and string to number) relates to converting signed numbers.

However, consider using unsigned conversion where possible.

#### `fio_atol10`

```c
int64_t fio_atol10(char **pstr);
```

Reads a signed base 10 formatted number.

#### `fio_atol`

```c
int64_t fio_atol(char **pstr);
```

A helper function that converts between String data to a signed int64_t.

Numbers are assumed to be in base 10. Octal (`0###`), Hex (`0x##`/`x##`) and binary (`0b##`/ `b##`) are recognized as well. For binary Most Significant Bit must come first.

The most significant difference between this function and `strtol` (aside of API design), is the added support for binary representations.

#### `fio_ltoa`

```c
size_t fio_ltoa(char *dest, int64_t num, uint8_t base);
```

A helper function that writes a signed int64_t to a `NUL` terminated string.

If `dest` is `NULL`, returns the number of bytes that would have been written.

No overflow guard is provided, make sure there's at least 68 bytes available (for base 2).

Offers special support for base 2 (binary), base 8 (octal), base 10 and base 16 (hex) where prefixes are automatically added if required (i.e., `"0x"` for hex, `"0b"` for base 2, and `"0"` for octal).

Supports any base up to base 36 (using 0-9,A-Z).

An unsupported base will log an error and print zero.

Returns the number of bytes actually written (excluding the NUL terminator).

#### `fio_atof`

```c
double fio_atof(char **pstr);
```

A helper function that converts between String data to a signed double.

Currently wraps `strtod` with some special case handling.

#### `fio_ftoa`

```c
size_t fio_ftoa(char *dest, double num, uint8_t base);
```

A helper function that converts between a double to a string.

Currently wraps `snprintf` with some special case handling.

No overflow guard is provided, make sure there's at least 130 bytes available (for base 2).

Supports base 2, base 10 and base 16. An unsupported base will silently default to base 10. Prefixes aren't added (i.e., no `"0x"` or `"0b"` at the beginning of the string).

Returns the number of bytes actually written (excluding the NUL terminator).

#### `fio_ltoa10`

```c
void fio_ltoa10(char *dest, int64_t i, size_t digits);
```

Writes a signed number to `dest` using `digits` bytes (+ `NUL`). See also [`fio_digits10`](#fio_digits10).

### Unsigned Number / String Conversion

#### `fio_ltoa10u`

```c
void fio_ltoa10u(char *dest, uint64_t i, size_t digits);
```

Writes an unsigned number to `dest` using `digits` bytes (+ `NUL`).

#### `fio_ltoa8u`

```c
void fio_ltoa8u(char *dest, uint64_t i, size_t digits);
```

Writes an unsigned number to `dest` using `digits` bytes (+ `NUL`) in octal format (base 8).

#### `fio_ltoa16u`

```c
void fio_ltoa16u(char *dest, uint64_t i, size_t digits);
```

Writes an unsigned number to `dest` using `digits` bytes (+ `NUL`) in hex format (base 16).

**Note**: for hex based numerals facil.io assumes that `digits` are always even (2, 4, 6, 8, 10, 12, 14, 16).

#### `fio_ltoa_bin`

```c
void fio_ltoa_bin(char *dest, uint64_t i, size_t digits);
```

Writes an unsigned number to `dest` using `digits` bytes (+ `NUL`) in binary format (base 2).

#### `fio_ltoa_xbase`

```c
void fio_ltoa_xbase(char *dest, uint64_t i, size_t digits, size_t base);
```

Writes an unsigned number to `dest` using `digits` bytes (+ `NUL`) in `base` format (up to base 36 inclusive).

#### `fio_atol8u`

```c
uint64_t fio_atol8u(char **pstr);
```

Reads an unsigned base 8 formatted number.

#### `fio_atol10u`

```c
uint64_t fio_atol10u(char **pstr);
```

Reads an unsigned base 10 formatted number.

#### `fio_atol16u`

```c
uint64_t fio_atol16u(char **pstr);
```

Reads an unsigned hex formatted number (possibly prefixed with `"0x"`).

#### `fio_atol_bin`

```c
uint64_t fio_atol_bin(char **pstr);
```

Reads an unsigned binary formatted number (possibly prefixed with `"0b"`).

#### `fio_atol_xbase`

```c
uint64_t fio_atol_xbase(char **pstr, size_t base);
```

Read an unsigned number in any base up to base 36.

### Number / String Conversion Helpers

#### `fio_c2i`

```c
uint8_t fio_c2i(unsigned char c);
```

Maps characters to alphanumerical value, where numbers have their natural values (`0-9`) and `A-Z` (or `a-z`) map to the values `10-35`.

Out of bound values return 255.

This allows calculations for up to base 36.

#### `fio_i2c`

```c
uint8_t fio_i2c(unsigned char i);
```

Maps numeral values to alphanumerical characters, where numbers have their natural values (`0-9`) and `A-Z` are the values `10-35`.

Accepts values up to 63. Returns zero for values over 35. Out of bound values produce undefined behavior.

This allows printing of numerals for up to base 36.

#### `fio_u2i_limit`

```c
int64_t fio_u2i_limit(uint64_t val, size_t to_negative);
```

Converts an unsigned `val` to a signed `val`, limiting the value to provide overflow protection and limiting it to either a negative or a positive value.

#### `fio_digits10`

```c
size_t fio_digits10(int64_t i);
```

Returns the number of digits of the **signed** number when using base 10. The result includes the possible sign (`-`) digit.

This function can be used before allocating memory in order to predict the amount of memory required by a String representation of the number.

#### `fio_digits10u`

```c
size_t fio_digits10u(uint64_t i);
```

Returns the number of digits of the **unsigned** number when using base 10.

This function can be used before allocating memory in order to predict the amount of memory required by a String representation of the number.

#### `fio_digits8u`

```c
size_t fio_digits8u(uint64_t i);
```

Returns the number of digits of the **unsigned** number when using base 8.

This function can be used before allocating memory in order to predict the amount of memory required by a String representation of the number.

#### `fio_digits16u`

```c
size_t fio_digits16u(uint64_t i);
```

Returns the number of digits in base 16 for an **unsigned** number.

Base 16 digits are always computed in pairs (byte sized chunks). Possible values are 2, 4, 6, 8, 10, 12, 14 and 16.

This function can be used before allocating memory in order to predict the amount of memory required by a String representation of the number.

**Note**: facil.io always assumes all base 16 numeral representations are printed as they are represented in memory.

#### `fio_digits_bin`

```c
size_t fio_digits_bin(uint64_t i);
```

Returns the number of digits of the **unsigned** number when using base 2.

This function can be used before allocating memory in order to predict the amount of memory required by a String representation of the number.

#### `fio_digits_xbase`

```c
size_t fio_digits_xbase(uint64_t i, size_t base);
```

Returns the number of digits of the **unsigned** number when using base `base`.

This function can be used before allocating memory in order to predict the amount of memory required by a String representation of the number.

### IEEE 754 Floating Point Helpers

#### `fio_i2d`

```c
double fio_i2d(int64_t mant, int64_t exponent_in_base_2);
```

Converts a 64 bit signed integer mantissa and a base-2 exponent to an IEEE 754 formatted double.

#### `fio_u2d`

```c
double fio_u2d(uint64_t mant, int64_t exponent_in_base_2);
```

Converts a 64 bit unsigned integer mantissa and a base-2 exponent to an IEEE 754 formatted double.

### Big Number Conversion

These functions provide hex string conversion for large unsigned integer types.

#### `fio_u128_hex_read`

```c
fio_u128 fio_u128_hex_read(char **pstr);
```

Reads a hex numeral string and initializes a 128-bit unsigned integer.

#### `fio_u256_hex_read`

```c
fio_u256 fio_u256_hex_read(char **pstr);
```

Reads a hex numeral string and initializes a 256-bit unsigned integer.

#### `fio_u512_hex_read`

```c
fio_u512 fio_u512_hex_read(char **pstr);
```

Reads a hex numeral string and initializes a 512-bit unsigned integer.

#### `fio_u1024_hex_read`

```c
fio_u1024 fio_u1024_hex_read(char **pstr);
```

Reads a hex numeral string and initializes a 1024-bit unsigned integer.

#### `fio_u2048_hex_read`

```c
fio_u2048 fio_u2048_hex_read(char **pstr);
```

Reads a hex numeral string and initializes a 2048-bit unsigned integer.

#### `fio_u4096_hex_read`

```c
fio_u4096 fio_u4096_hex_read(char **pstr);
```

Reads a hex numeral string and initializes a 4096-bit unsigned integer.

#### `fio_u128_hex_write`

```c
size_t fio_u128_hex_write(char *dest, const fio_u128 *u);
```

Writes a 128-bit unsigned integer to `dest` as a hex string.

Returns the number of bytes written (excluding the NUL terminator).

#### `fio_u256_hex_write`

```c
size_t fio_u256_hex_write(char *dest, const fio_u256 *u);
```

Writes a 256-bit unsigned integer to `dest` as a hex string.

Returns the number of bytes written (excluding the NUL terminator).

#### `fio_u512_hex_write`

```c
size_t fio_u512_hex_write(char *dest, const fio_u512 *u);
```

Writes a 512-bit unsigned integer to `dest` as a hex string.

Returns the number of bytes written (excluding the NUL terminator).

#### `fio_u1024_hex_write`

```c
size_t fio_u1024_hex_write(char *dest, const fio_u1024 *u);
```

Writes a 1024-bit unsigned integer to `dest` as a hex string.

Returns the number of bytes written (excluding the NUL terminator).

#### `fio_u2048_hex_write`

```c
size_t fio_u2048_hex_write(char *dest, const fio_u2048 *u);
```

Writes a 2048-bit unsigned integer to `dest` as a hex string.

Returns the number of bytes written (excluding the NUL terminator).

#### `fio_u4096_hex_write`

```c
size_t fio_u4096_hex_write(char *dest, const fio_u4096 *u);
```

Writes a 4096-bit unsigned integer to `dest` as a hex string.

Returns the number of bytes written (excluding the NUL terminator).

-------------------------------------------------------------------------------
