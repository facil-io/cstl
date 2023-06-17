/* *****************************************************************************

                                  Hash Maps


Easily define key-value String Hash Map, also sometimes called a "dictionary",
using different smart defaults for short keys `FIO_MAP_KEY_KSTR` vs longer keys
(or when expecting a sparsely populated map) `FIO_MAP_KEY_BSTR`.

***************************************************************************** */

/* *****************************************************************************
(2) Hash Map: String Dictionary (automatic setup)
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
  printf("- Printing each key: value\n");
  printf("Hello: %s\n", dict_get(&dictionary, FIO_STR_INFO1("Hello")).buf);
  printf("42:    %s\n", dict_get(&dictionary, FIO_STR_INFO1("42")).buf);
  /* update using dict_set */
  printf("- Updating value for 42 and reprinting\n");
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
(2) Hash Map: String Dictionary (manual setup)
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
(3) Hash Map: Strings to Numbers
***************************************************************************** */

/* map words to numbers. */
#define FIO_UMAP_NAME umap
#define FIO_MAP_KEY_KSTR
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
    printf("%.*s: %llu\n",
           (int)pos.key.len,
           pos.key.buf,
           (unsigned long long)value);
  }
  umap_destroy(&map);
}

/* *****************************************************************************
main
***************************************************************************** */

int main(void) {
  printf("=====================================\n");
  easy_dict_example();
  printf("=====================================\n");
  dictionary_example();
  printf("=====================================\n");
  map_keystr_example();
  printf("=====================================\n");
}
