# facil.io 0.8.x for C - now with an integrated C STL (Server Toolbox library)

[![POSIX C/C++ CI](https://github.com/facil-io/cstl/actions/workflows/c-cpp.yml/badge.svg)](https://github.com/facil-io/cstl/actions/workflows/c-cpp.yml) [![Windows C/C++ CI](https://github.com/facil-io/cstl/actions/workflows/windows.yml/badge.svg)](https://github.com/facil-io/cstl/actions/workflows/windows.yml)

The [facil.io library](https://facil.io) is much more than a Web Application Framework and includes core tools and type templates that any C (and C++) project will find useful.

In addition to useful helpers, [facil.io](https://facil.io) allows developers to use MACROS to generate code for different common types, such as Hash Maps, Arrays, Binary-Safe Strings, etc'.

In other words, some of the most common building blocks one would need in any C project are placed in this convenient header file library.

### Installing

Simply copy the `fio-stl.h` file to your project's folder (using a single header file).  Done.

Or... copy the `fio-stl` folder to your project's folder (using `"fio-stl/include.h"`). Done.

Include the file as many times as required and enjoy.

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

The fecil.io C STL provides a number of solutions for binary-safe dynamic Strings.

One approach (`fio_bstr`) is similar to the [Simple Dynamic Strings library](https://github.com/antirez/sds) (and with similar cons / pros), only providing more functionality.

For example, `fio_bstr` provides reference counted immutable strings with a "copy-on-write" fallback for when a string has to be mutated, making these String objects a perfect choice for cached Strings.


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
#include "fio-stl/include.h" /* or "fio-stl.h" */

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

### Hash Map Binary Safe Strings

Easily define key-value String Hash Map, also sometimes called a "dictionary", using different smart defaults for short keys `FIO_MAP_KEY_KSTR` vs longer keys (or when expecting a sparsely populated map) `FIO_MAP_KEY_BSTR`.

```c
#include "fio-stl/include.h" /* or "fio-stl.h" */

/* Set the properties for the key-value Unordered Map type called `dict_s` */
#define FIO_MAP_NAME       dict
#define FIO_MAP_KEY_KSTR   /* pre-defined macro for using fio_keystr_s keys. */
#define FIO_MAP_VALUE_BSTR /* pre-defined macro for using String values. */
#define FIO_MAP_HASH_FN(str)                                                   \
  fio_risky_hash(str.buf, str.len, (uint64_t)&fio_risky_hash)
#include FIO_INCLUDE_FILE /* subsequent include statements should prefer MACRO */

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

### Hash Map Any Type

Define your own Hash Maps for any key-value pair of any type.

In this example we manually construct a dictionary hash map where short String objects are mapped to other (often longer) String objects.

```c
#include "fio-stl/include.h" /* or "fio-stl.h" */

/* Create a binary safe String type for Strings that aren't mutated often */
#define FIO_STR_SMALL str
#include FIO_INCLUDE_FILE /* subsequent include statements should prefer MACRO */

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


### JSON and Soft Types

Languages such as JavaScript and Ruby allow developers to mix types easily and naturally, such as having an Array of mixed numeral and String types.

The facil.io library offers a solution that allows developers to do the same in C, using soft types and pointer tagging.

```c
#define FIO_FIOBJ
#include FIO_INCLUDE_FILE

void json_example(void) {
  /* we will create a hash map to contain all our key-value pairs */
  FIOBJ map = fiobj_hash_new();
  /* note that the ownership of `array` is placed inside the hash map. */
  FIOBJ array = fiobj_hash_set3(map, "array", 5, fiobj_array_new());
  for (size_t i = 0; i < 5; ++i) {
    /* add numerals to array */
    fiobj_array_unshift(array, fiobj_num_new(0 - (intptr_t)i));
    fiobj_array_push(array, fiobj_num_new((intptr_t)i + 1));
  }
  /* add a string to the array - note that the array will own the new String */
  fiobj_array_push(array, fiobj_str_new_cstr("done", 4));
  FIOBJ json_output = fiobj2json(FIOBJ_INVALID, map, 1);
  printf("JSON output of our hash:\n%s\n", fiobj_str_ptr(json_output));
  /* free the Hash Map - will free also all values (array, strings) and keys */
  fiobj_free(map);
  /* rebuild the map object from the JSON String */
  FIOBJ json_data = fiobj_json_parse(fiobj2cstr(json_output), NULL);
  /* free the JSON String */
  fiobj_free(json_output);
  /* seek an object in a complex object using a JSON style lookup string. */
  printf(
      "The 7th object in the \"array\" key is: %s\n",
      fiobj2cstr(fiobj_json_find(json_data, FIO_STR_INFO2("array[6]", 8))).buf);
  /* free the rebuilt map */
  fiobj_free(json_data);
}
```

### HTTP / WebSocket Server

And, of course, the HTTP / WebSocket / EventSource (SSE) and pub/sub Server is included.

Here's a simple static file server:

```c
#define FIO_CLI  /* provide a CLI interface for selecting public folder name */
#define FIO_HTTP /* HTTP server / parse and handle modules */
#define FIO_LOG  /* always log */
#include FIO_INCLUDE_FILE

/* This function handles HTTP requests, which is the same as the default. */
FIO_SFUNC void on_request(fio_http_s *h) {
  /* sends a static 404.html file, if exists. */
  fio_http_send_error_response(h, 404); 
}

/* The main function. */
int main(int argc, char const *argv[]) {
  /* provide a CLI interface for selecting the public folder */
  fio_cli_start(argc,
                argv,
                0,
                1,
                "a simple HTTP static file server. Use, for example:\n"
                "\tNAME -www ./public\n"
                "\tNAME ./public\n",
                FIO_CLI_STRING("--public-folder -www (.) the public root folder for static files."));
  if (fio_cli_unnamed_count())
    fio_cli_set("-www", fio_cli_unnamed(0));

  /* assert that the public folder is indeed a folder */
  FIO_ASSERT(fio_filename_is_folder(fio_cli_get("-www")),
             "not a folder:\n\t%s",
             fio_cli_get("-www"));

  /* listen for HTTP connections (and test for error) */
  FIO_ASSERT(fio_http_listen(NULL,
                             .public_folder = FIO_STR_INFO1((char *)fio_cli_get("-www")),
                             .on_http = on_request,
                             .log = 1),
             "Couldn't listen for HTTP connections.");
  fio_srv_start(0);
  return 0;
}
```

## Contribution Notice

If you're submitting a PR, make sure to update the corresponding code slice (file) in the `fio-stl` folder, the `makefile` will re-produce the `fio-stl.h` file automatically.

Note that the master branch is currently as unstable as it gets. Commits may get squashed, the branch may be overwritten (force push), etc'. I will play nicer when the code stabilizes.

Also, contributions are subject to the terms and conditions set in [the facil.io contribution guide](CONTRIBUTING.md). 

## Documentation

[Documentation is available in the (auto-generated) `fio-stl.md` file](fio-stl.md).
