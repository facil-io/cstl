# facil.io - C STL - a Simple Template Library for C

[![C/C++ CI](https://github.com/facil-io/cstl/workflows/C/C++%20CI/badge.svg)](https://github.com/facil-io/cstl/actions)

At the core of the [facil.io library](https://facil.io) is its powerful Simple Template Library for C (and C++).

The Simple Template Library is a single file library ([`fio-stl.h`](./fio-stl.h)), that uses MACROS to generate code for different common types, such as Hash Maps, Arrays, Linked Lists, Binary-Safe Strings, etc'.

In addition, the Simple Template Library offers common functional primitives and helpers, such as bit operations, atomic operations, CLI parsing, JSON, task queues, and custom memory allocators.

In other words, some of the most common building blocks one would need in any C project are placed in this single header file.

The header could be included multiple times with different results, creating different types or exposing different functionality.

### OS Support

The library in written and tested on POSIX systems. Windows support was added afterwards, leaving the library with a POSIX oriented design.

Please note I cannot continually test the windows support as I avoid the OS... hence, Windows OS support should be considered unstable.

### Installing

Simply copy the `fio-stl.h` file to your project's folder. Done.

### Running Tests

To test the STL locally you need to first fork the project or download the whole project source code. Then, from the project's root folder run:

```bash
make test/stl
```

The GNU `make` command will compile and run any file in the `tests` folder if it is explicitly listed. i.e.,

```bash
make test/malloc      # speed test facil.io's memory allocator
make test/json        # test JSON roundtrip with external JSON files
make test/json_minify # JSON minification example
make test/cpp         # Test template compilation in a C++ file (no run)... may fail on some compilers
```

It is possible to use the same `makefile` to compile source code and static library code. See the makefile for details.

On Windows you might want to skip the makefile (if you do not have `make` and `gcc` installed) and run:

```dos
cls && cl /Ox tests\stl.c /I. && stl.exe 
```

## Quick Examples

### Binary-Safe Dynamic Strings

The fecil.io C STL provides a builtin solution similar in approach to the [Simple Dynamic Strings library](https://github.com/antirez/sds) (and with similar disadvantages):

```c
/* include Core String functionality */
#define FIO_STR
#include <fio-stl.h>

void hello_binary_strings(void){
  char * bstr = fio_bstr_write(NULL, "Hello World", 11);
  /* note that `bstr` might be updated, not updating the pointer after a `write` is a bug. */
  bstr = fio_bstr_write2(bstr,
                         FIO_STRING_WRITE_STR1("\nThe answer is: "),
                         FIO_STRING_WRITE_UNUM(42));
  printf("%s\n", bstr);
  fio_bstr_free(bstr);
}
```

### Reference Counting Binary-Safe Dynamic Strings

Easily use a template to create your own binary safe String type that is always `NUL` terminated. Optionally add reference counting to your type with a single line of code.

Or use a template to create your own String type, much better for reference counting:

```c
/* Create a binary safe String type called `my_str_s` */
#define FIO_STR_NAME my_str
/* Use a reference counting for `my_str_s` (using the same name convention) */
#define FIO_REF_NAME my_str
/* Make the reference counter the only constructor rather then having it as an additional flavor */
#define FIO_REF_CONSTRUCTOR_ONLY
#include "fio-stl.h"

void hello(void){
  my_str_s * msg = my_str_new();
  my_str_write(msg, "Hello World", 11);
  /* increase reference */
  my_str_dup(msg);
  printf("%s\n", my_str2ptr(msg));
  my_str_free(msg);
  printf("Still valid, as we had 2 references: %s\n", my_str2ptr(msg));
  my_str_free(msg);
}
```

### Dynamic Arrays

Easily construct dynamic Array types for any type.

```c
typedef struct {
  int i;
  float f;
} foo_s;

#define FIO_ARRAY_NAME foo_ary
#define FIO_ARRAY_TYPE foo_s
#define FIO_ARRAY_TYPE_CMP(a,b) (a.i == b.i && a.f == b.f)
#include "fio-stl.h"

void example(void) {
  foo_ary_s a = FIO_ARRAY_INIT;
  foo_s *p = foo_ary_push(&a, (foo_s){.i = 42});
  FIO_ARRAY_EACH(foo_ary, &a, pos) { // pos will be a pointer to the element
    fprintf(stderr, "* [%zu]: %p : %d\n", (size_t)(pos - foo_ary2ptr(&a)), pos->i);
  }
  foo_ary_destroy(&a);
}
```

### Hash Maps

This is an example for a key-value String Hash Map, also sometimes called a "dictionary".

```c
/* Create a binary safe String type for Strings that aren't mutated often */
#define FIO_STR_SMALL str
#include "fio-stl.h"

/* Set the properties for the key-value Unordered Map type called `dict_s` */
#define FIO_MAP_NAME                 dict
#define FIO_MAP_ORDERED              0
#define FIO_MAP_TYPE                 str_s
#define FIO_MAP_TYPE_COPY(dest, src) str_init_copy2(&(dest), &(src))
#define FIO_MAP_TYPE_DESTROY(k)      str_destroy(&k)
#define FIO_MAP_TYPE_CMP(a, b)       str_is_eq(&(a), &(b))
#define FIO_MAP_KEY                  FIO_MAP_TYPE
#define FIO_MAP_KEY_COPY             FIO_MAP_TYPE_COPY
#define FIO_MAP_KEY_DESTROY          FIO_MAP_TYPE_DESTROY
#define FIO_MAP_KEY_CMP              FIO_MAP_TYPE_CMP
#include "fio-stl.h"
/** set helper for consistent hash values */
FIO_IFUNC str_s dict_set2(dict_s *m, str_s key, str_s obj) {
  return dict_set(m, str_hash(&key, (uint64_t)m), key, obj, NULL);
}
/** get helper for consistent hash values */
FIO_IFUNC str_s *dict_get2(dict_s *m, str_s key) {
  return dict_get_ptr(m, str_hash(&key, (uint64_t)m), key);
}

void example(void) {
  dict_s dictionary = FIO_MAP_INIT;
  str_s key, val;
  str_init_const(&key, "hello", 5);
  str_init_const(&val, "Hello World!", 12);
  dict_set2(&dictionary, key, val);
  fprintf(stdout, "%s\n", str2ptr(dict_get2(&dictionary, key)));
  dict_destroy(&dictionary);
}
```

This same example actually has a shortcut that uses the dedicate `fio_keystr_s` type - a string type that is optimized for Hash Maps that use mostly (but not always) short string keys (less than 15 bytes per key on 64 bit systems):


```c
/* map words to numbers. */
#define FIO_MAP_KEYSTR
#define FIO_UMAP_NAME umap
#define FIO_MAP_TYPE  uintptr_t
#include "fio-stl.h"

/** a helper to calculate hash and set any string as a key. */
FIO_IFUNC void umap_set2(umap_s *map, char *key, size_t key_len, uintptr_t obj) {
  umap_set(map, fio_risky_hash(key, key_len, (uint64_t)map), fio_keystr(key, key_len), obj, NULL);
}
/** a helper to calculate hash and set a constant string as a key. */
FIO_IFUNC void umap_set3(umap_s *map, char *key, size_t key_len, uintptr_t obj) {
  umap_set(map, fio_risky_hash(key, key_len, (uint64_t)map), fio_keystr_const(key, key_len), obj, NULL);
}

/** a helper to calculate hash and get the value of a key. */
FIO_IFUNC uintptr_t umap_get2(umap_s *map, char *key, size_t key_len) {
  uint64_t hash = fio_risky_hash(key, key_len, (uint64_t)map);
  return umap_get(map, hash, fio_keystr(key, key_len));
}
/* example adding strings to map and printing data. */
void example(void) {
  umap_s map = FIO_MAP_INIT;
  umap_set3(&map, "One", 3, 1); /* use `umap_set3` since this is a `const char *` string */
  umap_set3(&map, "Two", 3, 2);
  umap_set3(&map, "Three", 5, 3);
  FIO_MAP_EACH(umap, &map, pos) {
    /* note that key strings are NOT nul terminated! (minimizes allocations) */
    fio_buf_info_s key = fio_keystr_info(&pos->obj.key);
    uintptr_t value = pos->obj.value;
    printf("%.*s: %llu\n", (int)key.len, key.buf, (unsigned long long)value);
  }
  umap_destroy(&map);
  return 0;
}
```

## Contribution Notice

If you're submitting a PR, make sure to update the corresponding code slice (file) in the `stl_slices` folder.

Note that the master branch is unstable as it gets. Commits may get squashed, the branch may be overwritten (force push), etc'.

Also, contributions are subject to the terms and conditions set in [the facil.io contribution guide](https://github.com/boazsegev/facil.io/CONTRIBUTING.md). 

## Documentation

[Documentation is available in the (auto-generated) `fio-stl.md` file](fio-stl.md).
