# Dynamic Types — Module Group 200

The 200-range modules provide the core dynamic/generic types in the facil.io C STL: strings, arrays, maps, reference counting, and the FIOBJ soft-type system.

All five modules share the same design: **define a name macro, then include the header**. The preprocessor does the rest, generating a fully typed, prefixed API from static-inline functions and macros.

---

## The Pattern

```c
/* 1. Set the name (and any optional configuration macros). */
#define FIO_STR_NAME my_str

/* 2. Include the STL (single-header or multi-header). */
#include "fio-stl.h"

/* 3. Use the generated API — everything is prefixed with your name. */
my_str_s s = {0};
my_str_write(&s, "hello", 5);
my_str_destroy(&s);
```

Each inclusion is independent. You can define the same module multiple times in the same translation unit as long as you use a different name macro each time.

---

## Defining a Custom Type

| Name macro | What you get |
|---|---|
| `FIO_STR_NAME` | Binary-safe dynamic string type and API |
| `FIO_ARRAY_NAME` | Dynamic array for an element type you choose |
| `FIO_MAP_NAME` | Hash map or set (ordered or unordered) |
| `FIO_REF_NAME` | Reference-counted wrapper around any type |
| `FIO_FIOBJ` | Soft/dynamic object system (no extra name needed) |

Optional companion macros (e.g. `FIO_ARRAY_TYPE`, `FIO_MAP_KEY`, `FIO_MAP_VALUE`, `FIO_REF_TYPE`) let you configure element types, comparison functions, copy/destroy hooks, and more — consult the individual module docs for the full list.

---

## Modules

| Module | Name macro | Doc |
|---|---|---|
| Dynamic Strings | `FIO_STR_NAME` (or `FIO_STR_SMALL`) | [→ 201 string.md](./201 string.md) |
| Dynamic Arrays | `FIO_ARRAY_NAME` | [→ 202 array.md](./202 array.md) |
| Hash Maps / Sets | `FIO_MAP_NAME` | [→ 210 map.md](./210 map.md) |
| Reference Counting | `FIO_REF_NAME` | [→ 249 reference counter.md](./249 reference counter.md) |
| FIOBJ Soft Types | `FIO_FIOBJ` | [→ 250 fiobj.md](./250 fiobj.md) |

> **Note:** `210 map2.h` is a legacy file kept for backward compatibility and is not part of this active module group.

---

## Quick Example — Three Types, One File

```c
/* A small immutable-optimized string */
#define FIO_STR_SMALL tag_str
#include "fio-stl.h"

/* A dynamic array of those strings */
#define FIO_ARRAY_NAME tag_list
#define FIO_ARRAY_TYPE tag_str_s
#define FIO_ARRAY_TYPE_DESTROY(s) tag_str_destroy(&(s))
#include "fio-stl.h"

/* A map from string keys to integer values */
#define FIO_MAP_NAME   word_count
#define FIO_MAP_KEY_KSTR          /* use fio_keystr_s keys */
#define FIO_MAP_VALUE  size_t
#include "fio-stl.h"
```

Each `#include` is processed independently. Define → include → use. That's it.
