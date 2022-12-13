#define FIO_LOG
#define FIO_CLI
#include "fio-stl.h" /* from: https://github.com/facil-io/cstl */
#define FIO_FIOBJ
#include "fio-stl.h" /* from: https://github.com/facil-io/cstl */

int main(int argc, char const *argv[]) {
  // select the JSON file.
  fio_cli_start(argc,
                argv,
                2,
                2,
                "NAME will extract a data field from a JSON file using "
                "Javascript notation.\nUse:\n\tNAME <filename> <JSON_field>",
                FIO_CLI_PRINT_LINE("Example:"),
                FIO_CLI_PRINT("NAME users.json \\\"users[0].name\\\""));

  // open and read the JSON fiile
  FIOBJ json = fiobj_str_new();
  FIO_LOG_DEBUG2("attempting open:%s", fio_cli_unnamed(0));
  fio_str_info_s result = fiobj_str_readfile(json, fio_cli_unnamed(0), 0, 0);
  FIO_ASSERT(result.buf, "Couldn't open file %s", fio_cli_unnamed(0));

  // parse the JSON data
  FIO_LOG_DEBUG2("attempting to parse:\n%s", fiobj_str2cstr(json).buf);
  size_t consumed = 0;
  FIOBJ data = fiobj_json_parse(fiobj_str2cstr(json), &consumed);
  FIO_ASSERT(data != FIOBJ_INVALID, "couldn't parse data.");
  fiobj_free(json); /* we don't need the JSON file content anymore. */

  // re-format the JSON - example (replacing key-value pairs with arrays)
  json = fiobj_json_find2(data,
                          (char *)fio_cli_unnamed(1),
                          strlen(fio_cli_unnamed(1)));
  fiobj_dup(json);
  fiobj_free(data); /* we don't need the original object structure */
  data = fiobj2json(FIOBJ_INVALID, json, 1);

  /* print out the final data in JSON format. */
  printf("%s\n", fiobj_str_ptr(data));

  /* cleanup. */
  fiobj_free(data);
  fiobj_free(json);
  fio_cli_end();
  return 0;
}
