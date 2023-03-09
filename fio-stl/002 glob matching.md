## Globe Matching

```c
#define FIO_GLOB_MATCH
#include "fio-stl.h"
```

By defining the macro `FIO_GLOB_MATCH` the following functions are defined:

#### `fio_glob_match`

```c
uint8_t fio_glob_match(fio_str_info_s pat, fio_str_info_s str);
```

This function is a **binary** glob matching helper.

Returns 1 on a match, otherwise returns 0.

The following patterns are recognized:

* `*` - matches any string, including an empty string.
		
	i.e., the following patterns will match against the string `"String"`:

    `"*"`

    `"*String*"`

    `"S*ing"`

* `?` - matches any single **byte** (does NOT support UTF-8 characters).
		
	i.e., the following patterns will match against the string `"String"`:

    `"?tring"`

    `"Strin?"`

    `"St?ing"`

* `[!...]` or `[^...]` - matches any **byte** that is **not** withing the brackets (does **not** support UTF-8 characters).

    Byte ranges are supported using `'-'` (i.e., `[!0-9]`)

	Use the backslash (`\`) to escape the special `]`, `-` and `\` characters when they are part of the list.
	
	i.e., the following patterns will match against the string `"String"`:

    `"[!a-z]tring"`

    `"[^a-z]tring"`

    `"[^F]tring"` (same as `"[!F]tring"`)

* `[...]` - matches any **byte** that **is** withing the brackets (does **not** support UTF-8 characters).

	Use the backslash (`\`) to escape the special `]`, `-` and `\` characters when they are part of the list.
	
	i.e., the following patterns will match against the string `"String"`:

    `"[A-Z]tring"`

    `"[sS]tring"`


-------------------------------------------------------------------------------

