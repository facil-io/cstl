#define FIO_EVERYTHING
#include "fio-stl/include.h"

static void print_help(const char *nm) {
  printf("Please add a pattern and a string to test for match.\n"
         "\t%s \"[0-9]+\" \"0123456789\"\n",
         nm);
}
int main(int argc, char const *argv[]) {
  if (argc != 3) {
    print_help(argv[0]);
    return 1;
  }
  int r = fio_glob_match(FIO_STR_INFO1((char *)argv[1]),
                         FIO_STR_INFO1((char *)argv[2]));
  printf("\t- %s\n", r ? "Successful Match" : "NO MATCH!");
  return 0;
}
