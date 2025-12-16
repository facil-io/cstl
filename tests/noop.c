#define FIO_LOG
#define FIO_EVERYTHING
#define FIO_EXTERN          99
#define FIO_EXTERN_COMPLETE 99
#include "fio-stl/include.h"

int main(int argc, char const *argv[]) {
  (void)argc, (void)argv;
  FIO_LOG_DEBUG("Doing Nothing");
}
