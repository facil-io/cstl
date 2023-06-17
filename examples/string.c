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

int main(void) {
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
