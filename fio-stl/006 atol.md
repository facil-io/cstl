## String / Number conversion

```c
#define FIO_ATOL
#include "fio-stl.h"
```

If the `FIO_ATOL` macro is defined, the following functions will be defined:

**Note**: all functions that write to a buffer also write a `NUL` terminator byte.

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

No overflow guard is provided, so either allow for plenty of headroom (at least 65 bytes) or pass `NULL` first and allocate appropriately.

**Note**: special base prefixes for base 2 (binary) and base 16 (hex) are **NOT** added automatically. Consider adding any required prefix when possible (i.e.,`"0x"` for hex and `"0b"` for base 2).

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

No overflow guard is provided, make sure there's at least 130 bytes available
(for base 2).

Supports base 2, base 10 and base 16. An unsupported base will silently default
to base 10. Prefixes aren't added (i.e., no "0x" or "0b" at the beginning of the
string).

Returns the number of bytes actually written (excluding the NUL terminator).

#### `fio_ltoa10`

```c
void fio_ltoa10(char *dest, int64_t i, size_t digits);
```

Writes a signed number to `dest` using `digits` bytes (+ `NUL`). See also [`fio_digits10`](#fio_digits10).

### Unsigned Number / String Conversion

#### `fio_ltoa10`

```c
void fio_ltoa10(char *dest, uint64_t i, size_t digits);
```

Writes a signed number to `dest` using `digits` bytes (+ `NUL`).

#### `fio_ltoa10u`

```c
void fio_ltoa10u(char *dest, uint64_t i, size_t digits);
```

Writes an unsigned number to `dest` using `digits` bytes (+ `NUL`).

#### `fio_ltoa16u`

```c
void fio_ltoa16u(char *dest, uint64_t i, size_t digits);
```

Writes an unsigned number to `dest` using `digits` bytes (+ `NUL`) in hex format (base 16).

Note: for hex based numeral facil.io assumes that `digits` are always even (2, 4, 6, 8, 10, 12, 14, 16).

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

Reads an unsigned hex formatted number (possibly prefixed with "0x").

#### `fio_atol_bin`

```c
uint64_t fio_atol_bin(char **pstr);
```

Reads an unsigned binary formatted number (possibly prefixed with "0b").

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
size_t fio_digits10u(int64_t i);
```

Returns the number of digits of the **unsigned** number when using base 10.

This function can be used before allocating memory in order to predict the amount of memory required by a String representation of the number.

#### `fio_digits8u`

```c
size_t fio_digits8u(int64_t i);
```

Returns the number of digits of the **unsigned** number when using base 8.

This function can be used before allocating memory in order to predict the amount of memory required by a String representation of the number.

#### `fio_digits16u`

```c
size_t fio_digits16u(uint64_t i);
```

Returns the number of digits in base 16 for an **unsigned** number.

Base 16 digits are always computed in pairs (byte sized chunks). Possible values are 2,4,6,8,10,12,14 and 16.

This function can be used before allocating memory in order to predict the amount of memory required by a String representation of the number.

**Note**: facil.io always assumes all base 16 numeral representations are printed as they are represented in memory.

#### `fio_digits_bin`

```c
size_t fio_digits_bin(int64_t i);
```

Returns the number of digits of the **unsigned** number when using base 2.

This function can be used before allocating memory in order to predict the amount of memory required by a String representation of the number.

#### `fio_digits_xbase`

```c
size_t fio_digits_xbase(int64_t i);
```

Returns the number of digits of the **unsigned** number when using base `base`.

This function can be used before allocating memory in order to predict the amount of memory required by a String representation of the number.
-------------------------------------------------------------------------------
