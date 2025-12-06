/* *****************************************************************************

                        Using binary strings with `fio_bstr`

The fecil.io C STL provides a builtin solution similar in approach to the
[Simple Dynamic Strings library](https://github.com/antirez/sds) (and with
similar cons / pros):

***************************************************************************** */
#define FIO_LEAK_COUNTER 1

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

/* *****************************************************************************

            Reference Counting Binary-Safe Dynamic Strings

Easily use a template to create your own binary safe String type that is always
`NUL` terminated. Optionally add reference counting to your type with a single
line of code.

Note: unlike `copy`, a `dup` operation duplicates the handle, not the data.
***************************************************************************** */

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
  /* increase reference - duplicates the handle, but the data is shared(!) */
  my_str_s *ref = my_str_dup(msg);
  my_str_write(ref, ", written to both handles.", 26);
  printf("%s\n", my_str_ptr(msg));
  printf("%s\n", my_str_ptr(ref));
  my_str_free(msg);
  printf("Still valid, as we had 2 references:\n\t%s\n", my_str_ptr(ref));
  my_str_free(ref);
}

/* *****************************************************************************
Easily construct dynamic Array types for any type, including structs and unions.
***************************************************************************** */

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
    printf("* [%zu]: %p : %d\n",
           (size_t)(pos - foo_ary2ptr(&a)),
           (void *)pos,
           pos->i);
  }
  foo_ary_destroy(&a);
}

/* *****************************************************************************

                                  Hash Maps


Easily define key-value String Hash Map, also sometimes called a "dictionary",
using different smart defaults for short keys `FIO_MAP_KEY_KSTR` vs longer keys
(or when expecting a sparsely populated map) `FIO_MAP_KEY_BSTR`.

***************************************************************************** */

/* Set the properties for the key-value Unordered Map type called `dict_s` */
#define FIO_UMAP_NAME      dict
#define FIO_MAP_KEY_KSTR   /* pre-defined macro for using fio_keystr_s keys. */
#define FIO_MAP_VALUE_BSTR /* pre-defined macro for using String values. */
#define FIO_MAP_HASH_FN(str)                                                   \
  fio_risky_hash(str.buf, str.len, (uint64_t)&fio_risky_hash)
#include "fio-stl/include.h" /* or "fio-stl.h" */

void easy_dict_example(void) {
  dict_s dictionary = FIO_MAP_INIT;
  /* insertion using dict_set */
  dict_set(&dictionary,
           FIO_STR_INFO1("Hello"),
           FIO_STR_INFO1("Hello World!"),
           NULL);
  dict_set(&dictionary,
           FIO_STR_INFO1("42"),
           FIO_STR_INFO1("Meaning of life..."),
           NULL);
  /* access using dict_get */
  printf("Hello: %s\n", dict_get(&dictionary, FIO_STR_INFO1("Hello")).buf);
  printf("42:    %s\n", dict_get(&dictionary, FIO_STR_INFO1("42")).buf);
  /* update using dict_set */
  dict_set(&dictionary,
           FIO_STR_INFO1("42"),
           FIO_STR_INFO1("What was the question?"),
           NULL);
  /* map iteration - this is an unordered map, order is incidental. */
  FIO_MAP_EACH(dict, &dictionary, i) {
    printf("%-8s: %s\n", i.key.buf, i.value.buf);
  }
  /* removal using dict_remove */
  dict_remove(&dictionary, FIO_STR_INFO1("42"), NULL);
  /* Since the "42" key was removed, its `buf` value will point to NULL. */
  printf("Did we remove 42 ... ? - %s\n",
         dict_get(&dictionary, FIO_STR_INFO1("42")).buf
             ? dict_get(&dictionary, FIO_STR_INFO1("42")).buf
             : "removed");
  /* Cleanup. */
  dict_destroy(&dictionary);
}

/* *****************************************************************************

                                  Hash Maps


Define your own Hash Maps for any key-value pair of any type.

In this example we manually construct a dictionary hash map where short String
objects are mapped to other (often longer) String objects.
***************************************************************************** */

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

/* *****************************************************************************
Example
***************************************************************************** */

/* map words to numbers. */
#define FIO_MAP_KEY_KSTR
#define FIO_UMAP_NAME umap
#define FIO_MAP_VALUE uintptr_t
#define FIO_MAP_HASH_FN(k)                                                     \
  fio_risky_hash((k).buf, (k).len, (uint64_t)(uintptr_t)&umap_destroy)
#include "fio-stl/include.h" /* or "fio-stl.h" */

/* example adding strings to map and printing data. */
void map_keystr_example(void) {
  umap_s map = FIO_MAP_INIT;
  /* FIO_KEYSTR_CONST prevents copying of longer constant strings */
  umap_set(&map, FIO_STR_INFO3("One", 3, FIO_KEYSTR_CONST), 1, NULL);
  umap_set(&map, FIO_STR_INFO3("Two", 3, FIO_KEYSTR_CONST), 2, NULL);
  umap_set(&map, FIO_STR_INFO3("Three", 5, FIO_KEYSTR_CONST), 3, NULL);
  FIO_MAP_EACH(umap, &map, pos) {
    uintptr_t value = pos.value;
    /* note that key strings are NOT nul terminated! (minimizes allocations) */
    printf("%.*s: %llu\n",
           (int)pos.key.len,
           pos.key.buf,
           (unsigned long long)value);
  }
  umap_destroy(&map);
}

/* *****************************************************************************
String Core docs include this example
***************************************************************************** */

/* map words to numbers. */
#define FIO_UMAP_NAME umap_w2i
#define FIO_MAP_KEY_KSTR
#define FIO_MAP_VALUE uintptr_t
#define FIO_MAP_HASH_FN(k)                                                     \
  fio_risky_hash((k).buf, (k).len, (uint64_t)(uintptr_t)&umap_w2i_destroy)
#include "fio-stl/include.h" /* or "fio-stl.h" */

/* example adding strings to map and printing data. */
void example_in_string_core(void) {
  umap_w2i_s map = FIO_MAP_INIT;
  umap_w2i_set(&map, FIO_STR_INFO1("One"), 1, NULL);
  umap_w2i_set(&map, FIO_STR_INFO1("Two"), 2, NULL);
  umap_w2i_set(&map, FIO_STR_INFO1("Three"), 3, NULL);
  umap_w2i_set(&map, FIO_STR_INFO1("Infinity"), (uintptr_t)-1, NULL);
  FIO_MAP_EACH(umap_w2i, &map, i) {
    printf("%.*s: %llu\n",
           (int)i.key.len,
           i.key.buf,
           (unsigned long long)i.value);
  }
  umap_w2i_destroy(&map);
}

/* *****************************************************************************
JSON example
***************************************************************************** */
#define FIO_FIOBJ
#include FIO_INCLUDE_FILE

void json_example(void) {
  /* we will create a hash map to contain all our key-value pairs */
  FIOBJ map = fiobj_hash_new();
  /* note that the ownership of `array` is placed inside the hash map. */
  FIOBJ array = fiobj_hash_set2(map, "array", 5, fiobj_array_new());
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

/* *****************************************************************************
Server example
***************************************************************************** */
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
int http_example(int argc, char const *argv[]) {
  /* provide a CLI interface for selecting the public folder */
  fio_cli_start(argc,
                argv,
                0,
                1,
                "a simple HTTP static file server. Use, for example:\n"
                "\tNAME -www ./public\n"
                "\tNAME ./public\n",
                FIO_CLI_STRING("--public-folder -www (.) "
                               "the public root folder for static files."));
  if (fio_cli_unnamed_count())
    fio_cli_set("-www", fio_cli_unnamed(0));

  /* assert that the public folder is indeed a folder */
  FIO_ASSERT(fio_filename_is_folder(fio_cli_get("-www")),
             "not a folder:\n\t%s",
             fio_cli_get("-www"));

  /* listen for HTTP connections (and test for error) */
  FIO_ASSERT(fio_http_listen(NULL,
                             .public_folder =
                                 FIO_STR_INFO1((char *)fio_cli_get("-www")),
                             .on_http = on_request,
                             .log = 0),
             "Couldn't listen for HTTP connections.");
  fio_io_start(0);
  return 0;
}

/* *****************************************************************************
main
***************************************************************************** */

int main(int argc, char const *argv[]) {
  printf("=====================================\n");
  hello_binary_strings();
  printf("=====================================\n");
  reference_counted_shared_strings();
  printf("=====================================\n");
  array_example();
  printf("=====================================\n");
  easy_dict_example();
  printf("=====================================\n");
  dictionary_example();
  printf("=====================================\n");
  map_keystr_example();
  printf("=====================================\n");
  example_in_string_core();
  printf("=====================================\n");
  json_example();
  printf("=====================================\n");
  http_example(argc, argv);
  printf("=====================================\n");
}
