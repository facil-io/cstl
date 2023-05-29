#define FIO_URL
#include "fio-stl.h"

int main(int argc, char const *argv[]) {
  char *to_parse =
      (char *)"http://anon:1234@example.com:443/my/path?answer=42#target";
  if (argc >= 2)
    to_parse = (char *)argv[1];
  fio_url_s u = fio_url_parse(to_parse, strlen(to_parse));
  fprintf(stderr,
          "Parsed URL:\n"
          "\tscheme:\t %.*s\n"
          "\tuser:\t%.*s\n"
          "\tpass:\t%.*s\n"
          "\thost:\t%.*s\n"
          "\tport:\t%.*s\n"
          "\tpath:\t%.*s\n"
          "\tquery:\t%.*s\n"
          "\ttarget:\t%.*s\n",
          (int)u.scheme.len,
          u.scheme.buf,
          (int)u.user.len,
          u.user.buf,
          (int)u.password.len,
          u.password.buf,
          (int)u.host.len,
          u.host.buf,
          (int)u.port.len,
          u.port.buf,
          (int)u.path.len,
          u.path.buf,
          (int)u.query.len,
          u.query.buf,
          (int)u.target.len,
          u.target.buf);
  if (u.query.buf) {
    printf("Parsed query data:\n");
    FIO_URL_QUERY_EACH(u.query, i) {
      printf("\t%.*s = %.*s\n",
             (int)i.name.len,
             i.name.buf,
             (int)i.value.len,
             i.value.buf);
    }
  }
  return 0;
}
