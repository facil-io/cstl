# facil.io - C STL - a Simple Template Library for C

[![C/C++ CI](https://github.com/facil-io/cstl/workflows/C/C++%20CI/badge.svg)](https://github.com/facil-io/cstl/actions)

At the core of the [facil.io library](https://facil.io) is its powerful Simple Template Library for C (and C++).

The Simple Template Library is a single file library ([`fio-stl.h`](./fio-stl.h)), that uses MACROS to generate code for different common types, such as Hash Maps, Arrays, Linked Lists, Binary-Safe Strings, etc'.

In addition, the Simple Template Library offers common functional primitives and helpers, such as bit operations, atomic operations, CLI parsing, JSON, task queues, and a custom memory allocator.

In other words, all the common building blocks one could need in a C project are placed in this single header file.

The header could be included multiple times with different results, creating different types or exposing different functionality.

### Running Tests

Testing the STL locally is easy using:

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

## Quick Examples

### Binary-Safe Dynamic Strings

Easily construct binary safe String types that are always `NUL` terminated just in case you want to use them as a C String.

```c
/* Create a binary safe String type called `my_str_s` */
#define FIO_STR_NAME my_str
#include <fio-stl.h>

void hello(void){
  my_str_s msg = FIO_STR_INIT;
  my_str_write(&msg, "Hello World", 11);
  printf("%s\n", my_str2ptr(&msg));
  my_str_destroy(&msg);
}
```

### Dynamic Arrays

Easily construct dynamic Array types for any type of collection.

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

### Reference Counting

```c
/* Create a binary safe String type called `my_str_s` with reference counting */
#define FIO_STR_NAME my_str
#define FIO_REF_NAME my_str
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

### Hash Maps

This is an example for a key-value String Hash Map, also sometimes called a "dictionary".

```c
/* Create a binary safe String type for Strings that aren't mutated often */
#define FIO_STR_SMALL str
#include "fio-stl.h"

/* Set the properties for the key-value Unordered Map type called `dict_s` */
#define FIO_UMAP_NAME                dict
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


## Contribution Notice

If you're submitting a PR, make sure to update the corresponding code slice (file) in the `stl_slices` folder.

Note that the master branch is unstable as it gets. Commits may get squashed, the branch may be overwritten (force push), etc'.

Also, contributions are subject to the terms and conditions set in [the facil.io contribution guide](https://github.com/boazsegev/facil.io/CONTRIBUTING.md). 

## Documentation

[Documentation is available in the (auto-generated) `fio-stl.md` file](fio-stl.md).
