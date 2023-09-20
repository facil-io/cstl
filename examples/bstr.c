/* *****************************************************************************

                        Using binary strings with `fio_bstr`

The fecil.io C STL provides a builtin solution similar in approach to the
Simple Dynamic Strings library (https://github.com/antirez/sds)...
... and with similar cons / pros:

***************************************************************************** */
#define FIO_LEAK_COUNTER 1

/* include Core String functionality */
#define FIO_STR
#include "fio-stl/include.h" /* or "fio-stl.h" */

int main(void) {
  /* note that the `bstr` pointer must always be updated!
   * not updating the pointer after a `write` operation is a bug. */
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
