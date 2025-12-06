/* *****************************************************************************
Test
***************************************************************************** */
#define FIO_CLI
#include "test-helpers.h"

int main(void) {
  const char *argv[] = {
      "appname",
      "-i11",
      "-i2=2",
      "-i3",
      "3",
      "-t,u",
      "-s",
      "test",
      "unnamed",
  };
  const int argc = sizeof(argv) / sizeof(argv[0]);
  { /* avoid macro for C++ */
    fio___cli_line_s arguments[] = {
        FIO_CLI_INT("-integer1 -i1 first integer"),
        FIO_CLI_INT("-integer2 -i2 second integer"),
        FIO_CLI_INT("-integer3 -i3 third integer"),
        FIO_CLI_INT("-integer4 -i4 (4) fourth integer"),
        FIO_CLI_INT("-integer5 -i5 (\"5\") fifth integer"),
        FIO_CLI_BOOL("-boolean -t boolean"),
        FIO_CLI_BOOL("-boolean2 -u boolean"),
        FIO_CLI_BOOL("-boolean_false -f boolean"),
        FIO_CLI_STRING("-str -s a string"),
        FIO_CLI_PRINT_HEADER("Printing stuff"),
        FIO_CLI_PRINT_LINE("does nothing, but shouldn't crash either"),
        FIO_CLI_PRINT("does nothing, but shouldn't crash either"),
        {(fio_cli_arg_e)0},
    };
    fio_cli_start FIO_NOOP(argc, argv, 0, -1, NULL, arguments);
  }
  FIO_ASSERT(fio_cli_get_i("-i2") == 2, "CLI second integer error.");
  FIO_ASSERT(fio_cli_get_i("-i3") == 3, "CLI third integer error.");
  FIO_ASSERT(fio_cli_get_i("-i4") == 4,
             "CLI fourth integer error (%s).",
             fio_cli_get("-i4"));
  FIO_ASSERT(fio_cli_get_i("-i5") == 5,
             "CLI fifth integer error (%s).",
             fio_cli_get("-i5"));
  FIO_ASSERT(fio_cli_get_i("-i1") == 1, "CLI first integer error.");
  FIO_ASSERT(fio_cli_get_i("-i2") == fio_cli_get_i("-integer2"),
             "CLI second integer error.");
  FIO_ASSERT(fio_cli_get_i("-i3") == fio_cli_get_i("-integer3"),
             "CLI third integer error.");
  FIO_ASSERT(fio_cli_get_i("-i1") == fio_cli_get_i("-integer1"),
             "CLI first integer error.");
  FIO_ASSERT(fio_cli_get_i("-t") == 1, "CLI boolean true error.");
  FIO_ASSERT(fio_cli_get_i("-u") == 1, "CLI boolean 2 true error.");
  FIO_ASSERT(fio_cli_get_i("-f") == 0, "CLI boolean false error.");
  FIO_ASSERT(!strcmp(fio_cli_get("-s"), "test"), "CLI string error.");
  FIO_ASSERT(fio_cli_unnamed_count() == 1, "CLI unnamed count error.");
  FIO_ASSERT(!strcmp(fio_cli_unnamed(0), "unnamed"), "CLI unnamed error.");
  fio_cli_set("-manual", "okay");
  FIO_ASSERT(!strcmp(fio_cli_get("-manual"), "okay"), "CLI set/get error.");
  fio_cli_end();
  FIO_ASSERT(fio_cli_get_i("-i1") == 0, "CLI cleanup error.");
}
