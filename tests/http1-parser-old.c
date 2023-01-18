#define FIO_TIME
#define FIO_LOG
#include "fio-stl/include.h"
#define HTTP1_TEST_PARSER 1
#include "http1-parser.h"

int main(void) {
  http1_parser_test();
  return 0;
}
