/* *****************************************************************************
Test
***************************************************************************** */
#define FIO_CLI
#include "test-helpers.h"

static int cli_count_positional(fio_buf_info_s name, fio_buf_info_s value,
                                fio_cli_arg_e type, void *udata) {
  size_t *count = (size_t *)udata;
  FIO_ASSERT(!name.buf && type == FIO_CLI_ARG_STRING && value.len,
             "CLI positional iterator type/name mismatch");
  ++*count;
  return 0;
}

static int cli_count_empty(fio_buf_info_s name, fio_buf_info_s value,
                           fio_cli_arg_e type, void *udata) {
  FIO_ASSERT(!name.buf && value.buf && !value.len &&
                 type == FIO_CLI_ARG_STRING,
             "CLI empty positional iteration mismatch");
  ++*(size_t *)udata;
  return 0;
}

static int cli_stop(fio_buf_info_s name, fio_buf_info_s value,
                    fio_cli_arg_e type, void *udata) {
  (void)name; (void)value; (void)type;
  ++*(size_t *)udata;
  return 1;
}

int main(void) {
  /* Embedded CLI copies are erased, but borrowed input remains untouched. */
  char borrowed[] = "borrowed-value-long-enough";
  fio_cli_str_s embedded = fio_cli_str_new(FIO_BUF_INFO1((char *)"secret"));
  fio_cli_str_s view = fio_cli_str_tmp(FIO_BUF_INFO1(borrowed));
  fio_cli_str_destroy(&embedded);
  fio_cli_str_destroy(&view);
  const unsigned char zeros[sizeof(embedded)] = {0};
  FIO_ASSERT(!memcmp(&embedded, zeros, sizeof(embedded)) &&
                 !memcmp(&view, zeros, sizeof(view)) &&
                 !strcmp(borrowed, "borrowed-value-long-enough"),
             "CLI embedded/borrowed clear behavior error.");
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
        {.t = (fio_cli_arg_e)0},
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
  fio_cli_set("-i1", NULL);
  FIO_ASSERT(!fio_cli_get("-i1") && fio_cli_get_i("-i1") == 0,
             "CLI NULL should clear integer option.");
  fio_cli_set("-i2", "");
  FIO_ASSERT(!fio_cli_get("-i2") && fio_cli_get_i("-i2") == 0,
             "CLI empty string should clear integer option.");
  fio_cli_set("-s", "");
  FIO_ASSERT(!fio_cli_get("-s") && !fio_cli_get_str("-s").buf,
             "CLI empty string should clear named string option.");
  fio_cli_set("-s", "test");
  fio_cli_set_i("-i1", 0);
  FIO_ASSERT(fio_cli_get("-i1") && !strcmp(fio_cli_get("-i1"), "0"),
             "CLI explicit zero should remain present.");
  fio_cli_set("-t", NULL);
  FIO_ASSERT(!fio_cli_get_bool("-t"), "CLI NULL should clear boolean option.");
  fio_cli_set("-u", "");
  FIO_ASSERT(!fio_cli_get_bool("-u"), "CLI empty should clear boolean option.");
  FIO_ASSERT(fio_cli_unnamed_count() == 1, "CLI unnamed count error.");
  FIO_ASSERT(!strcmp(fio_cli_unnamed(0), "unnamed"), "CLI unnamed error.");
  fio_cli_set("-manual", "okay");
  FIO_ASSERT(!strcmp(fio_cli_get("-manual"), "okay"), "CLI set/get error.");
  fio_cli_set("-s", fio_cli_get("-s"));
  FIO_ASSERT(!strcmp(fio_cli_get("-s"), "test"), "CLI self assignment error.");
  size_t callbacks = 0;
  FIO_ASSERT(fio_cli_each(cli_stop, &callbacks) == 1 && callbacks == 1,
             "CLI cancellation must stop before positional values.");
  fio_cli_end();
  FIO_ASSERT(fio_cli_get_i("-i1") == 0, "CLI cleanup error.");

  for (unsigned int i = 0; i < 20; ++i)
    fio_cli_set_unnamed(i, "a long positional argument beyond inline storage");
  callbacks = 0;
  FIO_ASSERT(fio_cli_each(cli_count_positional, &callbacks) == 20 &&
                 callbacks == 20, "CLI array growth or positional iteration error.");
  const char *old = fio_cli_unnamed(0);
  fio_cli_set_unnamed(0, old);
  FIO_ASSERT(!strcmp(fio_cli_unnamed(0),
                     "a long positional argument beyond inline storage"),
             "CLI positional self assignment error.");
  unsigned int empty_index = fio_cli_set_unnamed(20, "");
  FIO_ASSERT(empty_index == 20 && fio_cli_unnamed_count() == 21 &&
                 fio_cli_unnamed(empty_index) &&
                 !strcmp(fio_cli_unnamed(empty_index), "") &&
                 fio_cli_unnamed_str(empty_index).buf &&
                 !fio_cli_unnamed_str(empty_index).len,
             "CLI empty unnamed argument should remain present.");
  FIO_ASSERT(fio_cli_set_unnamed(empty_index, NULL) == (unsigned int)-1 &&
                 fio_cli_unnamed(empty_index),
             "CLI NULL unnamed argument should not replace empty value.");
  callbacks = 0;
  FIO_ASSERT(fio_cli_each(cli_stop, &callbacks) == 1 && callbacks == 1,
             "CLI empty unnamed argument should be included in iteration.");
  const char *empty_argv[] = {"app", ""};
  fio_cli_start(2, empty_argv, 1, 1, "empty positional",
                FIO_CLI_PRINT_HEADER("Options"));
  FIO_ASSERT(fio_cli_unnamed_count() == 1 && fio_cli_unnamed(0) &&
                 !fio_cli_unnamed(0)[0],
             "CLI parser must preserve an empty unnamed argument.");
  callbacks = 0;
  FIO_ASSERT(fio_cli_each(cli_count_empty, &callbacks) == 1 && callbacks == 1,
             "CLI iterator must include the empty unnamed argument.");
  fio_cli_set(NULL, "");
  FIO_ASSERT(fio_cli_unnamed_count() == 2 && fio_cli_unnamed(1) &&
                 !fio_cli_unnamed(1)[0],
             "CLI set must append an empty unnamed argument.");
  const char *empty_int_argv[] = {"app", "-p", ""};
  fio_cli_start(3, empty_int_argv, 0, 0, "empty integer",
                FIO_CLI_INT("-p (42) integer option"));
  FIO_ASSERT(!fio_cli_get("-p") && fio_cli_get_i("-p") == 0,
             "CLI empty separate integer value should clear its default.");
  const char *empty_int_equals[] = {"app", "-p="};
  fio_cli_start(2, empty_int_equals, 0, 0, "empty integer",
                FIO_CLI_INT("-p (42) integer option"));
  FIO_ASSERT(!fio_cli_get("-p") && fio_cli_get_i("-p") == 0,
             "CLI empty equals integer value should clear its default.");
  const char *equals_then_value[] = {"app", "-p=", "17"};
  fio_cli_start(3, equals_then_value, 0, 1, "equals then positional",
                FIO_CLI_INT("-p (42) integer option"));
  FIO_ASSERT(!fio_cli_get("-p") && fio_cli_get_i("-p") == 0 &&
                 fio_cli_unnamed_count() == 1 &&
                 !strcmp(fio_cli_unnamed(0), "17"),
             "CLI trailing equals should clear without consuming next argument.");
  const char *again[] = {"another-app"};
  fio_cli_start(1, again, 0, 0, "another NAME", FIO_CLI_STRING("-new new option"));
  FIO_ASSERT(fio_cli_unnamed_count() == 0 && !fio_cli_get("-i1") &&
                 fio_cli_type("-new") == FIO_CLI_ARG_STRING,
             "CLI repeated start should reset old data.");
  fio_cli_end();
}
