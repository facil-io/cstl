## String / Number conversion

If the `FIO_ATOL` macro is defined, the following functions will be defined:

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

A helper function that writes a signed int64_t to a string.

No overflow guard is provided, make sure there's at least 68 bytes available
(for base 2).

Offers special support for base 2 (binary), base 8 (octal), base 10 and base 16
(hex). An unsupported base will silently default to base 10. Prefixes are
automatically added (i.e., "0x" for hex and "0b" for base 2).

Returns the number of bytes actually written (excluding the NUL terminator).


#### `fio_ftoa`

```c
size_t fio_ftoa(char *dest, double num, uint8_t base);
```

A helper function that converts between a double to a string.

Currently wraps `sprintf` with some special case handling.

No overflow guard is provided, make sure there's at least 130 bytes available
(for base 2).

Supports base 2, base 10 and base 16. An unsupported base will silently default
to base 10. Prefixes aren't added (i.e., no "0x" or "0b" at the beginning of the
string).

Returns the number of bytes actually written (excluding the NUL terminator).

-------------------------------------------------------------------------------
