## String / Number conversion

```c
#define FIO_ATOL
#include "fio-stl.h"
```

If the `FIO_ATOL` macro is defined, the following functions will be defined:

### String / Number Helpers

#### `fio_c2i`

```c
uint8_t fio_c2i(unsigned char c);
```

Maps characters to alphanumerical value, where numbers have their natural values (`0-9`) and `A-Z` (or `a-z`) map to the values `10-35`.

Out of bound values return 255.

This allows calculations for up to base 36.

#### `fio_digits10`

```c
size_t fio_digits10(int64_t i);
```

Returns the signed number of digits in base 10. This number includes the possible `-` sign digit.

#### `fio_digits10u`

```c
size_t fio_digits10u(uint64_t i);
```

Returns the number of digits in base 10 for an unsigned number.

#### `fio_digits16`

```c
size_t fio_digits16(uint64_t i);
```

Returns the number of digits in base 16 for an **unsigned** number.

Base 16 digits are always computed in pairs (byte sized chunks). Possible values are 2,4,6,8,10,12,14 and 16.

**Note**: facil.io always assumes all base 16 numeral representations are printed as they are represented in memory.

### String / Number Conversion API

#### `fio_atol`

```c
int64_t fio_atol(char **pstr);
```

A helper function that converts between String data to a signed int64_t.

Numbers are assumed to be in base 10. Octal (`0###`), Hex (`0x##`/`x##`) and
binary (`0b##`/ `b##`) are recognized as well. For binary Most Significant Bit
must come first.

The most significant difference between this function and `strtol` (aside of API
design), is the added support for binary representations.

#### `fio_atof`

```c
double fio_atof(char **pstr);
```

A helper function that converts between String data to a signed double.

Currently wraps `strtod` with some special case handling.

#### `fio_ltoa`

```c
size_t fio_ltoa(char *dest, int64_t num, uint8_t base);
```

A helper function that writes a signed int64_t to a `NUL` terminated string.

No overflow guard is provided, make sure there's at least 66 bytes available (i.e., for base 2).

**Note**: special base prefixes for base 2 (binary) and base 16 (hex) ar **NOT** added automatically. Consider adding any required prefix when possible (i.e.,`"0x"` for hex and `"0b"` for base 2).

Supports any base up to base 36 (using 0-9,A-Z).

An unsupported base will log an error and print zero.

Returns the number of bytes actually written (excluding the NUL terminator).

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

-------------------------------------------------------------------------------
