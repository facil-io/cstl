/* *****************************************************************************
Using JSON with FIOBJ (facil.io soft types)
***************************************************************************** */
#define FIO_FIOBJ
#include "fio-stl/include.h"

int main(void) {
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

  /* output data as a new JSON String */
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
