# facil.io - C STL - a Simple Template Library for C

[![POSIX C/C++ CI](https://github.com/facil-io/cstl/actions/workflows/c-cpp.yml/badge.svg)](https://github.com/facil-io/cstl/actions/workflows/c-cpp.yml) [![Windows C/C++ CI](https://github.com/facil-io/cstl/actions/workflows/windows.yml/badge.svg)](https://github.com/facil-io/cstl/actions/workflows/windows.yml)

At the core of the [facil.io library](https://facil.io) is its powerful Simple Template Library for C (and C++).

The Simple Template Library is a single file library ([`fio-stl.h`](./fio-stl.h)), that uses MACROS to generate code for different common types, such as Hash Maps, Arrays, Linked Lists, Binary-Safe Strings, etc'.

In addition, the Simple Template Library offers common functional primitives and helpers, such as bit operations, atomic operations, CLI parsing, JSON, task queues, and custom memory allocators.

In other words, some of the most common building blocks one would need in any C project are placed in this single header file.

The header could be included multiple times with different results, creating different types or exposing different functionality.

### OS Support

The library in written and tested on POSIX systems. Windows support was added afterwards, leaving the library with a POSIX oriented design.

Please note I cannot continually test the windows support as I avoid the OS... hence, Windows OS support should be considered unstable.

### Installing

Simply copy the `fio-stl.h` file to your project's folder (using a single header file). Done.

Or... copy the `fio-stl` folder to your project's folder (using `"fio-stl/include.h"`). Done.

### Running Tests

To test the STL locally you need to first fork the project or download the whole project source code. Then, from the project's root folder run:

```bash
make test/stl
```

The GNU `make` command will compile and run any file in the `tests` folder if it is explicitly listed. i.e.,

```bash
make tests/malloc      # speed test facil.io's memory allocator
make tests/json        # test JSON roundtrip with external JSON files
make tests/json_minify # JSON minification example
make tests/cpp         # Test template compilation in a C++ file (no run)... may fail on some compilers
```

It is possible to use the same `makefile` to compile source code and static library code. See the makefile for details.

On Windows you might want to skip the makefile (if you do not have `make` and `gcc` installed) and run:

```dos
cls && cl /Ox tests\stl.c /I. && stl.exe 
```

## Quick Examples

### Binary-Safe Dynamic Strings

The fecil.io C STL provides a builtin solution similar in approach to the [Simple Dynamic Strings library](https://github.com/antirez/sds) (and with similar cons / pros):

```c
/* include Core String functionality */
#define FIO_STR
#include "fio-stl/include.h" /* or "fio-stl.h" */

void hello_binary_strings(void) {
  /* note that the `bstr` pointer might be updated!
   * not updating the pointer after a `write` is a bug. */
  char *org = fio_bstr_write(NULL, "Hello World", 11);
  char *copy = fio_bstr_copy(org);
  printf("Since we use copy on write: %p == %p\n", (void *)copy, (void *)org);
  /* we could also use fio_bstr_printf, but `write2` should be faster. */
  copy = fio_bstr_write2(copy,
                         FIO_STRING_WRITE_STR1(". The answer is: "),
                         FIO_STRING_WRITE_UNUM(42));
  printf("Original string: %s\n", org);
  printf("Copied string:   %s\n", copy);
  fio_bstr_free(org);
  fio_bstr_free(copy);
}
```

### Reference Counting Binary-Safe Dynamic Strings

Easily use a template to create your own binary safe String type that is always `NUL` terminated.

Optionally add reference counting to your type with a single line of code.

**Note**: unlike `copy` which creates an independent copy, a `dup` operation duplicates the handle (does not copy the data), so both handles point to the same object.

```c
/* Create a binary safe String type called `my_str_s` */
#define FIO_STR_NAME my_str
/* Use a reference counting for `my_str_s` (using the same name convention) */
#define FIO_REF_NAME my_str
/* Make the reference counter the only constructor
 * rather then having it as an additional flavor */
#define FIO_REF_CONSTRUCTOR_ONLY
#include "fio-stl/include.h" /* or "fio-stl.h" */

void reference_counted_shared_strings(void) {
  my_str_s *msg = my_str_new();
  my_str_write(msg, "Hello World", 11);
  /* increase reference - duplicates the handle, but the string data is shared(!) */
  my_str_s *ref = my_str_dup(msg);
  my_str_write(ref, ", written to both handles.", 26);
  printf("%s\n", my_str_ptr(msg));
  printf("%s\n", my_str_ptr(ref));
  my_str_free(msg);
  printf("Still valid, as we had 2 references:\n\t%s\n", my_str_ptr(ref));
  my_str_free(ref);
}
```

### Dynamic Arrays

Easily construct dynamic Array types for any type.

```c
typedef struct {
  int i;
  float f;
} foo_s;

#define FIO_ARRAY_NAME           foo_ary
#define FIO_ARRAY_TYPE           foo_s
#define FIO_ARRAY_TYPE_CMP(a, b) (a.i == b.i && a.f == b.f)
#include "fio-stl.h"

void array_example(void) {
  foo_ary_s a = FIO_ARRAY_INIT;
  foo_ary_push(&a, (foo_s){.i = 42});
  FIO_ARRAY_EACH(foo_ary, &a, pos) { // pos will be a pointer to the element
    fprintf(stderr,
            "* [%zu]: %p : %d\n",
            (size_t)(pos - foo_ary2ptr(&a)),
            (void *)pos,
            pos->i);
  }
  foo_ary_destroy(&a);
}
```

### Hash Map Any Type

Define your own Hash Maps for any key-value pair of any type.

In this example we manually construct a dictionary hash map where short String objects are mapped to other (often longer) String objects.

```c
/* Create a binary safe String type for Strings that aren't mutated often */
#define FIO_STR_SMALL str
#include "fio-stl/include.h" /* or "fio-stl.h" */

/* Defines a key-value Unordered Map type called `dictionary_s` */
#define FIO_MAP_NAME                  dictionary
#define FIO_MAP_ORDERED               0
#define FIO_MAP_VALUE                 str_s
#define FIO_MAP_VALUE_COPY(dest, src) str_init_copy2(&(dest), &(src))
#define FIO_MAP_VALUE_DESTROY(k)      str_destroy(&k)
#define FIO_MAP_VALUE_CMP(a, b)       str_is_eq(&(a), &(b))
#define FIO_MAP_KEY                   FIO_MAP_VALUE
#define FIO_MAP_KEY_COPY              FIO_MAP_VALUE_COPY
#define FIO_MAP_KEY_DESTROY           FIO_MAP_VALUE_DESTROY
#define FIO_MAP_KEY_CMP               FIO_MAP_VALUE_CMP
#include "fio-stl/include.h" /* or "fio-stl.h" */
/** set helper for consistent hash values */
FIO_IFUNC str_s dictionary_set2(dictionary_s *m, str_s key, str_s obj) {
  return dictionary_set(m, str_hash(&key, (uint64_t)m), key, obj, NULL);
}
/** get helper for consistent hash values */
FIO_IFUNC str_s *dictionary_get2(dictionary_s *m, str_s key) {
  return &(dictionary_get_ptr(m, str_hash(&key, (uint64_t)m), key)->value);
}

void dictionary_example(void) {
  dictionary_s dictionary = FIO_MAP_INIT;
  str_s key, val;
  str_init_const(&key, "hello", 5);
  str_init_const(&val, "Hello World!", 12);
  dictionary_set2(&dictionary, key, val);
  printf("%s\n", str_ptr(dictionary_get2(&dictionary, key)));
  dictionary_destroy(&dictionary);
}
```

### Hash Map Binary Safe Strings

Easily define key-value String Hash Map, also sometimes called a "dictionary", using different smart defaults for short keys `FIO_MAP_KEY_KSTR` vs longer keys (or when expecting a sparsely populated map) `FIO_MAP_KEY_BSTR`.

```c
/* Set the properties for the key-value Unordered Map type called `dict_s` */
#define FIO_MAP_NAME       dict
#define FIO_MAP_KEY_KSTR   /* pre-defined macro for using fio_keystr_s keys. */
#define FIO_MAP_VALUE_BSTR /* pre-defined macro for using String values. */
#define FIO_MAP_HASH_FN(str)                                                   \
  fio_risky_hash(str.buf, str.len, (uint64_t)&fio_risky_hash)
#include "fio-stl/include.h" /* or "fio-stl.h" */

void easy_dict_example(void) {
  dict_s dictionary = FIO_MAP_INIT;
  dict_set(&dictionary,
           FIO_STR_INFO1("Hello"),
           FIO_STR_INFO1("Hello World!"),
           NULL);
  dict_set(&dictionary,
           FIO_STR_INFO1("42"),
           FIO_STR_INFO1("Meaning of life..."),
           NULL);
  printf("Hello: %s\n", dict_get(&dictionary, FIO_STR_INFO1("Hello")).buf);
  printf("42:    %s\n", dict_get(&dictionary, FIO_STR_INFO1("42")).buf);
  dict_destroy(&dictionary);
}
```

## Contribution Notice

If you're submitting a PR, make sure to update the corresponding code slice (file) in the `stl_slices` folder.

Note that the master branch is unstable as it gets. Commits may get squashed, the branch may be overwritten (force push), etc'.

Also, contributions are subject to the terms and conditions set in [the facil.io contribution guide](https://github.com/boazsegev/facil.io/CONTRIBUTING.md). 

## Documentation

[Documentation is available in the (auto-generated) `fio-stl.md` file](fio-stl.md).
