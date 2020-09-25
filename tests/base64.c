#define FIO_STR_NAME fio_str
#define FIO_CLI
#define FIO_SOCK
#include "fio-stl.h"

FIO_IFUNC fio_str_info_s fio_str_write_base64enc2(fio_str_s *s,
                                                  const void *data,
                                                  size_t len) {
  return fio_str_write_base64enc(s, data, len, fio_cli_get_bool("-u"));
}

int main(int argc, char const *argv[]) {
  fio_cli_start(
      argc,
      argv,
      0,
      -1,
      "Decode / Encode Base64 from stdin to stdout",
      FIO_CLI_STRING(
          "--file -f a filename to encode / decode (instead of stdin)."),
      FIO_CLI_BOOL("--decode -d run the program in decoding mode instead of "
                   "encoding mode."),
      FIO_CLI_BOOL("--url -u when (if) encoding, use URL mode."),
      FIO_CLI_PRINT_HEADER("Notes:"),
      FIO_CLI_PRINT_LINE(
          "When running NAME in the terminal with manual input, use "
          "^D (ctrl+D) to end session."),
      FIO_CLI_PRINT_HEADER("Examples:"),
      FIO_CLI_PRINT("NAME Hello World"),
      FIO_CLI_PRINT("NAME -f my_file.txt > my_file.base64"),
      FIO_CLI_PRINT("NAME -d -f my_file.base64 > my_file.txt"),
      FIO_CLI_PRINT("NAME -f my_file.base64 > file.txt"),
      FIO_CLI_PRINT("echo \"SGVsbG8gV29ybGQ=\" | NAME -d"),
      FIO_CLI_PRINT("NAME -d SGVsbG8gV29ybGQ="));
  fio_str_s out = FIO_STR_INIT;
  ;
  fio_str_info_s (*fn)(fio_str_s *, const void *, size_t) =
      fio_str_write_base64enc2;
  if (fio_cli_get_bool("-d"))
    fn = fio_str_write_base64dec;
  int fd = fileno(stdin);
  /* process Base64 as arguments */
  if (fio_cli_unnamed_count()) {
    for (size_t i = 0; i < fio_cli_unnamed_count(); ++i) {
      const char *str = fio_cli_unnamed(i);
      size_t len = strlen(str);
      fn(&out, str, len);
      fio_str_write(&out, "\n", 1);
    }
    fprintf(stdout, "%s", fio_str2ptr(&out));
    fio_str_resize(&out, 0);
    /* don't process stdio */
    if (!fio_cli_get("-f"))
      goto finish;
  }

  /* process Base64 as file */
  if (fio_cli_get("-f")) {
    fio_str_s in = FIO_STR_INIT;
    if (fio_str_readfile(&in, fio_cli_get("-f"), 0, 0).buf) {
      fn(&out, fio_str2ptr(&in), fio_str_len(&in));
      fio_str_destroy(&in);
      fprintf(stdout, "%s\n", fio_str2ptr(&out));
      fio_str_resize(&out, 0);
    } else {
      fprintf(stderr, "Couldn't load file: %s\n", fio_cli_get("-f"));
    }
    /* don't process stdio */
    goto finish;
  }
  fflush(stdout);
  /* process Base64 from STDIN */
  while (!feof(stdin)) {
    char buffer[960];
    ssize_t l = read(fd, buffer, 960);
    if (l <= 0)
      break;
    if (l && buffer[l - 1] == '\n')
      --l;
    if (l && buffer[l - 1] == '\r')
      --l;
    if (l)
      fn(&out, buffer, l);
    if (fio_str_len(&out)) {
      if (l < 960)
        fprintf(stdout, "%s\n", fio_str2ptr(&out));
      else
        fprintf(stdout, "%s", fio_str2ptr(&out));
    }
    fio_str_resize(&out, 0);
    fflush(stdout);
  }
finish:
  fprintf(stderr, "\n");
  fio_cli_end();
  fio_str_destroy(&out);
  return 0;
}
